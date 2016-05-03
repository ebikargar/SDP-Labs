4400 // File system implementation.  Five layers:
4401 //   + Blocks: allocator for raw disk blocks.
4402 //   + Log: crash recovery for multi-step updates.
4403 //   + Files: inode allocator, reading, writing, metadata.
4404 //   + Directories: inode with special contents (list of other inodes!)
4405 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
4406 //
4407 // This file contains the low-level file system manipulation
4408 // routines.  The (higher-level) system call implementations
4409 // are in sysfile.c.
4410 
4411 #include "types.h"
4412 #include "defs.h"
4413 #include "param.h"
4414 #include "stat.h"
4415 #include "mmu.h"
4416 #include "proc.h"
4417 #include "spinlock.h"
4418 #include "buf.h"
4419 #include "fs.h"
4420 #include "file.h"
4421 
4422 #define min(a, b) ((a) < (b) ? (a) : (b))
4423 static void itrunc(struct inode*);
4424 
4425 // Read the super block.
4426 void
4427 readsb(int dev, struct superblock *sb)
4428 {
4429   struct buf *bp;
4430 
4431   bp = bread(dev, 1);
4432   memmove(sb, bp->data, sizeof(*sb));
4433   brelse(bp);
4434 }
4435 
4436 // Zero a block.
4437 static void
4438 bzero(int dev, int bno)
4439 {
4440   struct buf *bp;
4441 
4442   bp = bread(dev, bno);
4443   memset(bp->data, 0, BSIZE);
4444   log_write(bp);
4445   brelse(bp);
4446 }
4447 
4448 
4449 
4450 // Blocks.
4451 
4452 // Allocate a zeroed disk block.
4453 static uint
4454 balloc(uint dev)
4455 {
4456   int b, bi, m;
4457   struct buf *bp;
4458   struct superblock sb;
4459 
4460   bp = 0;
4461   readsb(dev, &sb);
4462   for(b = 0; b < sb.size; b += BPB){
4463     bp = bread(dev, BBLOCK(b, sb.ninodes));
4464     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
4465       m = 1 << (bi % 8);
4466       if((bp->data[bi/8] & m) == 0){  // Is block free?
4467         bp->data[bi/8] |= m;  // Mark block in use.
4468         log_write(bp);
4469         brelse(bp);
4470         bzero(dev, b + bi);
4471         return b + bi;
4472       }
4473     }
4474     brelse(bp);
4475   }
4476   panic("balloc: out of blocks");
4477 }
4478 
4479 // Free a disk block.
4480 static void
4481 bfree(int dev, uint b)
4482 {
4483   struct buf *bp;
4484   struct superblock sb;
4485   int bi, m;
4486 
4487   readsb(dev, &sb);
4488   bp = bread(dev, BBLOCK(b, sb.ninodes));
4489   bi = b % BPB;
4490   m = 1 << (bi % 8);
4491   if((bp->data[bi/8] & m) == 0)
4492     panic("freeing free block");
4493   bp->data[bi/8] &= ~m;
4494   log_write(bp);
4495   brelse(bp);
4496 }
4497 
4498 
4499 
4500 // Inodes.
4501 //
4502 // An inode describes a single unnamed file.
4503 // The inode disk structure holds metadata: the file's type,
4504 // its size, the number of links referring to it, and the
4505 // list of blocks holding the file's content.
4506 //
4507 // The inodes are laid out sequentially on disk immediately after
4508 // the superblock. Each inode has a number, indicating its
4509 // position on the disk.
4510 //
4511 // The kernel keeps a cache of in-use inodes in memory
4512 // to provide a place for synchronizing access
4513 // to inodes used by multiple processes. The cached
4514 // inodes include book-keeping information that is
4515 // not stored on disk: ip->ref and ip->flags.
4516 //
4517 // An inode and its in-memory represtative go through a
4518 // sequence of states before they can be used by the
4519 // rest of the file system code.
4520 //
4521 // * Allocation: an inode is allocated if its type (on disk)
4522 //   is non-zero. ialloc() allocates, iput() frees if
4523 //   the link count has fallen to zero.
4524 //
4525 // * Referencing in cache: an entry in the inode cache
4526 //   is free if ip->ref is zero. Otherwise ip->ref tracks
4527 //   the number of in-memory pointers to the entry (open
4528 //   files and current directories). iget() to find or
4529 //   create a cache entry and increment its ref, iput()
4530 //   to decrement ref.
4531 //
4532 // * Valid: the information (type, size, &c) in an inode
4533 //   cache entry is only correct when the I_VALID bit
4534 //   is set in ip->flags. ilock() reads the inode from
4535 //   the disk and sets I_VALID, while iput() clears
4536 //   I_VALID if ip->ref has fallen to zero.
4537 //
4538 // * Locked: file system code may only examine and modify
4539 //   the information in an inode and its content if it
4540 //   has first locked the inode. The I_BUSY flag indicates
4541 //   that the inode is locked. ilock() sets I_BUSY,
4542 //   while iunlock clears it.
4543 //
4544 // Thus a typical sequence is:
4545 //   ip = iget(dev, inum)
4546 //   ilock(ip)
4547 //   ... examine and modify ip->xxx ...
4548 //   iunlock(ip)
4549 //   iput(ip)
4550 //
4551 // ilock() is separate from iget() so that system calls can
4552 // get a long-term reference to an inode (as for an open file)
4553 // and only lock it for short periods (e.g., in read()).
4554 // The separation also helps avoid deadlock and races during
4555 // pathname lookup. iget() increments ip->ref so that the inode
4556 // stays cached and pointers to it remain valid.
4557 //
4558 // Many internal file system functions expect the caller to
4559 // have locked the inodes involved; this lets callers create
4560 // multi-step atomic operations.
4561 
4562 struct {
4563   struct spinlock lock;
4564   struct inode inode[NINODE];
4565 } icache;
4566 
4567 void
4568 iinit(void)
4569 {
4570   initlock(&icache.lock, "icache");
4571 }
4572 
4573 static struct inode* iget(uint dev, uint inum);
4574 
4575 
4576 
4577 
4578 
4579 
4580 
4581 
4582 
4583 
4584 
4585 
4586 
4587 
4588 
4589 
4590 
4591 
4592 
4593 
4594 
4595 
4596 
4597 
4598 
4599 
4600 // Allocate a new inode with the given type on device dev.
4601 // A free inode has a type of zero.
4602 struct inode*
4603 ialloc(uint dev, short type)
4604 {
4605   int inum;
4606   struct buf *bp;
4607   struct dinode *dip;
4608   struct superblock sb;
4609 
4610   readsb(dev, &sb);
4611 
4612   for(inum = 1; inum < sb.ninodes; inum++){
4613     bp = bread(dev, IBLOCK(inum));
4614     dip = (struct dinode*)bp->data + inum%IPB;
4615     if(dip->type == 0){  // a free inode
4616       memset(dip, 0, sizeof(*dip));
4617       dip->type = type;
4618       log_write(bp);   // mark it allocated on the disk
4619       brelse(bp);
4620       return iget(dev, inum);
4621     }
4622     brelse(bp);
4623   }
4624   panic("ialloc: no inodes");
4625 }
4626 
4627 // Copy a modified in-memory inode to disk.
4628 void
4629 iupdate(struct inode *ip)
4630 {
4631   struct buf *bp;
4632   struct dinode *dip;
4633 
4634   bp = bread(ip->dev, IBLOCK(ip->inum));
4635   dip = (struct dinode*)bp->data + ip->inum%IPB;
4636   dip->type = ip->type;
4637   dip->major = ip->major;
4638   dip->minor = ip->minor;
4639   dip->nlink = ip->nlink;
4640   dip->size = ip->size;
4641   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
4642   log_write(bp);
4643   brelse(bp);
4644 }
4645 
4646 
4647 
4648 
4649 
4650 // Find the inode with number inum on device dev
4651 // and return the in-memory copy. Does not lock
4652 // the inode and does not read it from disk.
4653 static struct inode*
4654 iget(uint dev, uint inum)
4655 {
4656   struct inode *ip, *empty;
4657 
4658   acquire(&icache.lock);
4659 
4660   // Is the inode already cached?
4661   empty = 0;
4662   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
4663     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
4664       ip->ref++;
4665       release(&icache.lock);
4666       return ip;
4667     }
4668     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
4669       empty = ip;
4670   }
4671 
4672   // Recycle an inode cache entry.
4673   if(empty == 0)
4674     panic("iget: no inodes");
4675 
4676   ip = empty;
4677   ip->dev = dev;
4678   ip->inum = inum;
4679   ip->ref = 1;
4680   ip->flags = 0;
4681   release(&icache.lock);
4682 
4683   return ip;
4684 }
4685 
4686 // Increment reference count for ip.
4687 // Returns ip to enable ip = idup(ip1) idiom.
4688 struct inode*
4689 idup(struct inode *ip)
4690 {
4691   acquire(&icache.lock);
4692   ip->ref++;
4693   release(&icache.lock);
4694   return ip;
4695 }
4696 
4697 
4698 
4699 
4700 // Lock the given inode.
4701 // Reads the inode from disk if necessary.
4702 void
4703 ilock(struct inode *ip)
4704 {
4705   struct buf *bp;
4706   struct dinode *dip;
4707 
4708   if(ip == 0 || ip->ref < 1)
4709     panic("ilock");
4710 
4711   acquire(&icache.lock);
4712   while(ip->flags & I_BUSY)
4713     sleep(ip, &icache.lock);
4714   ip->flags |= I_BUSY;
4715   release(&icache.lock);
4716 
4717   if(!(ip->flags & I_VALID)){
4718     bp = bread(ip->dev, IBLOCK(ip->inum));
4719     dip = (struct dinode*)bp->data + ip->inum%IPB;
4720     ip->type = dip->type;
4721     ip->major = dip->major;
4722     ip->minor = dip->minor;
4723     ip->nlink = dip->nlink;
4724     ip->size = dip->size;
4725     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
4726     brelse(bp);
4727     ip->flags |= I_VALID;
4728     if(ip->type == 0)
4729       panic("ilock: no type");
4730   }
4731 }
4732 
4733 // Unlock the given inode.
4734 void
4735 iunlock(struct inode *ip)
4736 {
4737   if(ip == 0 || !(ip->flags & I_BUSY) || ip->ref < 1)
4738     panic("iunlock");
4739 
4740   acquire(&icache.lock);
4741   ip->flags &= ~I_BUSY;
4742   wakeup(ip);
4743   release(&icache.lock);
4744 }
4745 
4746 
4747 
4748 
4749 
4750 // Drop a reference to an in-memory inode.
4751 // If that was the last reference, the inode cache entry can
4752 // be recycled.
4753 // If that was the last reference and the inode has no links
4754 // to it, free the inode (and its content) on disk.
4755 void
4756 iput(struct inode *ip)
4757 {
4758   acquire(&icache.lock);
4759   if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
4760     // inode has no links: truncate and free inode.
4761     if(ip->flags & I_BUSY)
4762       panic("iput busy");
4763     ip->flags |= I_BUSY;
4764     release(&icache.lock);
4765     itrunc(ip);
4766     ip->type = 0;
4767     iupdate(ip);
4768     acquire(&icache.lock);
4769     ip->flags = 0;
4770     wakeup(ip);
4771   }
4772   ip->ref--;
4773   release(&icache.lock);
4774 }
4775 
4776 // Common idiom: unlock, then put.
4777 void
4778 iunlockput(struct inode *ip)
4779 {
4780   iunlock(ip);
4781   iput(ip);
4782 }
4783 
4784 
4785 
4786 
4787 
4788 
4789 
4790 
4791 
4792 
4793 
4794 
4795 
4796 
4797 
4798 
4799 
4800 // Inode content
4801 //
4802 // The content (data) associated with each inode is stored
4803 // in blocks on the disk. The first NDIRECT block numbers
4804 // are listed in ip->addrs[].  The next NINDIRECT blocks are
4805 // listed in block ip->addrs[NDIRECT].
4806 
4807 // Return the disk block address of the nth block in inode ip.
4808 // If there is no such block, bmap allocates one.
4809 static uint
4810 bmap(struct inode *ip, uint bn)
4811 {
4812   uint addr, *a;
4813   struct buf *bp;
4814 
4815   if(bn < NDIRECT){
4816     if((addr = ip->addrs[bn]) == 0)
4817       ip->addrs[bn] = addr = balloc(ip->dev);
4818     return addr;
4819   }
4820   bn -= NDIRECT;
4821 
4822   if(bn < NINDIRECT){
4823     // Load indirect block, allocating if necessary.
4824     if((addr = ip->addrs[NDIRECT]) == 0)
4825       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
4826     bp = bread(ip->dev, addr);
4827     a = (uint*)bp->data;
4828     if((addr = a[bn]) == 0){
4829       a[bn] = addr = balloc(ip->dev);
4830       log_write(bp);
4831     }
4832     brelse(bp);
4833     return addr;
4834   }
4835 
4836   panic("bmap: out of range");
4837 }
4838 
4839 
4840 
4841 
4842 
4843 
4844 
4845 
4846 
4847 
4848 
4849 
4850 // Truncate inode (discard contents).
4851 // Only called when the inode has no links
4852 // to it (no directory entries referring to it)
4853 // and has no in-memory reference to it (is
4854 // not an open file or current directory).
4855 static void
4856 itrunc(struct inode *ip)
4857 {
4858   int i, j;
4859   struct buf *bp;
4860   uint *a;
4861 
4862   for(i = 0; i < NDIRECT; i++){
4863     if(ip->addrs[i]){
4864       bfree(ip->dev, ip->addrs[i]);
4865       ip->addrs[i] = 0;
4866     }
4867   }
4868 
4869   if(ip->addrs[NDIRECT]){
4870     bp = bread(ip->dev, ip->addrs[NDIRECT]);
4871     a = (uint*)bp->data;
4872     for(j = 0; j < NINDIRECT; j++){
4873       if(a[j])
4874         bfree(ip->dev, a[j]);
4875     }
4876     brelse(bp);
4877     bfree(ip->dev, ip->addrs[NDIRECT]);
4878     ip->addrs[NDIRECT] = 0;
4879   }
4880 
4881   ip->size = 0;
4882   iupdate(ip);
4883 }
4884 
4885 // Copy stat information from inode.
4886 void
4887 stati(struct inode *ip, struct stat *st)
4888 {
4889   st->dev = ip->dev;
4890   st->ino = ip->inum;
4891   st->type = ip->type;
4892   st->nlink = ip->nlink;
4893   st->size = ip->size;
4894 }
4895 
4896 
4897 
4898 
4899 
4900 // Read data from inode.
4901 int
4902 readi(struct inode *ip, char *dst, uint off, uint n)
4903 {
4904   uint tot, m;
4905   struct buf *bp;
4906 
4907   if(ip->type == T_DEV){
4908     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
4909       return -1;
4910     return devsw[ip->major].read(ip, dst, n);
4911   }
4912 
4913   if(off > ip->size || off + n < off)
4914     return -1;
4915   if(off + n > ip->size)
4916     n = ip->size - off;
4917 
4918   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
4919     bp = bread(ip->dev, bmap(ip, off/BSIZE));
4920     m = min(n - tot, BSIZE - off%BSIZE);
4921     memmove(dst, bp->data + off%BSIZE, m);
4922     brelse(bp);
4923   }
4924   return n;
4925 }
4926 
4927 
4928 
4929 
4930 
4931 
4932 
4933 
4934 
4935 
4936 
4937 
4938 
4939 
4940 
4941 
4942 
4943 
4944 
4945 
4946 
4947 
4948 
4949 
4950 // Write data to inode.
4951 int
4952 writei(struct inode *ip, char *src, uint off, uint n)
4953 {
4954   uint tot, m;
4955   struct buf *bp;
4956 
4957   if(ip->type == T_DEV){
4958     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
4959       return -1;
4960     return devsw[ip->major].write(ip, src, n);
4961   }
4962 
4963   if(off > ip->size || off + n < off)
4964     return -1;
4965   if(off + n > MAXFILE*BSIZE)
4966     return -1;
4967 
4968   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
4969     bp = bread(ip->dev, bmap(ip, off/BSIZE));
4970     m = min(n - tot, BSIZE - off%BSIZE);
4971     memmove(bp->data + off%BSIZE, src, m);
4972     log_write(bp);
4973     brelse(bp);
4974   }
4975 
4976   if(n > 0 && off > ip->size){
4977     ip->size = off;
4978     iupdate(ip);
4979   }
4980   return n;
4981 }
4982 
4983 
4984 
4985 
4986 
4987 
4988 
4989 
4990 
4991 
4992 
4993 
4994 
4995 
4996 
4997 
4998 
4999 
5000 // Directories
5001 
5002 int
5003 namecmp(const char *s, const char *t)
5004 {
5005   return strncmp(s, t, DIRSIZ);
5006 }
5007 
5008 // Look for a directory entry in a directory.
5009 // If found, set *poff to byte offset of entry.
5010 struct inode*
5011 dirlookup(struct inode *dp, char *name, uint *poff)
5012 {
5013   uint off, inum;
5014   struct dirent de;
5015 
5016   if(dp->type != T_DIR)
5017     panic("dirlookup not DIR");
5018 
5019   for(off = 0; off < dp->size; off += sizeof(de)){
5020     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5021       panic("dirlink read");
5022     if(de.inum == 0)
5023       continue;
5024     if(namecmp(name, de.name) == 0){
5025       // entry matches path element
5026       if(poff)
5027         *poff = off;
5028       inum = de.inum;
5029       return iget(dp->dev, inum);
5030     }
5031   }
5032 
5033   return 0;
5034 }
5035 
5036 
5037 
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Write a new directory entry (name, inum) into the directory dp.
5051 int
5052 dirlink(struct inode *dp, char *name, uint inum)
5053 {
5054   int off;
5055   struct dirent de;
5056   struct inode *ip;
5057 
5058   // Check that name is not present.
5059   if((ip = dirlookup(dp, name, 0)) != 0){
5060     iput(ip);
5061     return -1;
5062   }
5063 
5064   // Look for an empty dirent.
5065   for(off = 0; off < dp->size; off += sizeof(de)){
5066     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5067       panic("dirlink read");
5068     if(de.inum == 0)
5069       break;
5070   }
5071 
5072   strncpy(de.name, name, DIRSIZ);
5073   de.inum = inum;
5074   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5075     panic("dirlink");
5076 
5077   return 0;
5078 }
5079 
5080 
5081 
5082 
5083 
5084 
5085 
5086 
5087 
5088 
5089 
5090 
5091 
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
5100 // Paths
5101 
5102 // Copy the next path element from path into name.
5103 // Return a pointer to the element following the copied one.
5104 // The returned path has no leading slashes,
5105 // so the caller can check *path=='\0' to see if the name is the last one.
5106 // If no name to remove, return 0.
5107 //
5108 // Examples:
5109 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5110 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5111 //   skipelem("a", name) = "", setting name = "a"
5112 //   skipelem("", name) = skipelem("////", name) = 0
5113 //
5114 static char*
5115 skipelem(char *path, char *name)
5116 {
5117   char *s;
5118   int len;
5119 
5120   while(*path == '/')
5121     path++;
5122   if(*path == 0)
5123     return 0;
5124   s = path;
5125   while(*path != '/' && *path != 0)
5126     path++;
5127   len = path - s;
5128   if(len >= DIRSIZ)
5129     memmove(name, s, DIRSIZ);
5130   else {
5131     memmove(name, s, len);
5132     name[len] = 0;
5133   }
5134   while(*path == '/')
5135     path++;
5136   return path;
5137 }
5138 
5139 
5140 
5141 
5142 
5143 
5144 
5145 
5146 
5147 
5148 
5149 
5150 // Look up and return the inode for a path name.
5151 // If parent != 0, return the inode for the parent and copy the final
5152 // path element into name, which must have room for DIRSIZ bytes.
5153 static struct inode*
5154 namex(char *path, int nameiparent, char *name)
5155 {
5156   struct inode *ip, *next;
5157 
5158   if(*path == '/')
5159     ip = iget(ROOTDEV, ROOTINO);
5160   else
5161     ip = idup(proc->cwd);
5162 
5163   while((path = skipelem(path, name)) != 0){
5164     ilock(ip);
5165     if(ip->type != T_DIR){
5166       iunlockput(ip);
5167       return 0;
5168     }
5169     if(nameiparent && *path == '\0'){
5170       // Stop one level early.
5171       iunlock(ip);
5172       return ip;
5173     }
5174     if((next = dirlookup(ip, name, 0)) == 0){
5175       iunlockput(ip);
5176       return 0;
5177     }
5178     iunlockput(ip);
5179     ip = next;
5180   }
5181   if(nameiparent){
5182     iput(ip);
5183     return 0;
5184   }
5185   return ip;
5186 }
5187 
5188 struct inode*
5189 namei(char *path)
5190 {
5191   char name[DIRSIZ];
5192   return namex(path, 0, name);
5193 }
5194 
5195 struct inode*
5196 nameiparent(char *path, char *name)
5197 {
5198   return namex(path, 1, name);
5199 }
