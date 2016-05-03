5400 //
5401 // File-system system calls.
5402 // Mostly argument checking, since we don't trust
5403 // user code, and calls into file.c and fs.c.
5404 //
5405 
5406 #include "types.h"
5407 #include "defs.h"
5408 #include "param.h"
5409 #include "stat.h"
5410 #include "mmu.h"
5411 #include "proc.h"
5412 #include "fs.h"
5413 #include "file.h"
5414 #include "fcntl.h"
5415 
5416 // Fetch the nth word-sized system call argument as a file descriptor
5417 // and return both the descriptor and the corresponding struct file.
5418 static int
5419 argfd(int n, int *pfd, struct file **pf)
5420 {
5421   int fd;
5422   struct file *f;
5423 
5424   if(argint(n, &fd) < 0)
5425     return -1;
5426   if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
5427     return -1;
5428   if(pfd)
5429     *pfd = fd;
5430   if(pf)
5431     *pf = f;
5432   return 0;
5433 }
5434 
5435 // Allocate a file descriptor for the given file.
5436 // Takes over file reference from caller on success.
5437 static int
5438 fdalloc(struct file *f)
5439 {
5440   int fd;
5441 
5442   for(fd = 0; fd < NOFILE; fd++){
5443     if(proc->ofile[fd] == 0){
5444       proc->ofile[fd] = f;
5445       return fd;
5446     }
5447   }
5448   return -1;
5449 }
5450 int
5451 sys_dup(void)
5452 {
5453   struct file *f;
5454   int fd;
5455 
5456   if(argfd(0, 0, &f) < 0)
5457     return -1;
5458   if((fd=fdalloc(f)) < 0)
5459     return -1;
5460   filedup(f);
5461   return fd;
5462 }
5463 
5464 int
5465 sys_read(void)
5466 {
5467   struct file *f;
5468   int n;
5469   char *p;
5470 
5471   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5472     return -1;
5473   return fileread(f, p, n);
5474 }
5475 
5476 int
5477 sys_write(void)
5478 {
5479   struct file *f;
5480   int n;
5481   char *p;
5482 
5483   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5484     return -1;
5485   return filewrite(f, p, n);
5486 }
5487 
5488 int
5489 sys_close(void)
5490 {
5491   int fd;
5492   struct file *f;
5493 
5494   if(argfd(0, &fd, &f) < 0)
5495     return -1;
5496   proc->ofile[fd] = 0;
5497   fileclose(f);
5498   return 0;
5499 }
5500 int
5501 sys_fstat(void)
5502 {
5503   struct file *f;
5504   struct stat *st;
5505 
5506   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
5507     return -1;
5508   return filestat(f, st);
5509 }
5510 
5511 // Create the path new as a link to the same inode as old.
5512 int
5513 sys_link(void)
5514 {
5515   char name[DIRSIZ], *new, *old;
5516   struct inode *dp, *ip;
5517 
5518   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
5519     return -1;
5520   if((ip = namei(old)) == 0)
5521     return -1;
5522 
5523   begin_trans();
5524 
5525   ilock(ip);
5526   if(ip->type == T_DIR){
5527     iunlockput(ip);
5528     commit_trans();
5529     return -1;
5530   }
5531 
5532   ip->nlink++;
5533   iupdate(ip);
5534   iunlock(ip);
5535 
5536   if((dp = nameiparent(new, name)) == 0)
5537     goto bad;
5538   ilock(dp);
5539   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
5540     iunlockput(dp);
5541     goto bad;
5542   }
5543   iunlockput(dp);
5544   iput(ip);
5545 
5546   commit_trans();
5547 
5548   return 0;
5549 
5550 bad:
5551   ilock(ip);
5552   ip->nlink--;
5553   iupdate(ip);
5554   iunlockput(ip);
5555   commit_trans();
5556   return -1;
5557 }
5558 
5559 // Is the directory dp empty except for "." and ".." ?
5560 static int
5561 isdirempty(struct inode *dp)
5562 {
5563   int off;
5564   struct dirent de;
5565 
5566   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
5567     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5568       panic("isdirempty: readi");
5569     if(de.inum != 0)
5570       return 0;
5571   }
5572   return 1;
5573 }
5574 
5575 
5576 
5577 
5578 
5579 
5580 
5581 
5582 
5583 
5584 
5585 
5586 
5587 
5588 
5589 
5590 
5591 
5592 
5593 
5594 
5595 
5596 
5597 
5598 
5599 
5600 int
5601 sys_unlink(void)
5602 {
5603   struct inode *ip, *dp;
5604   struct dirent de;
5605   char name[DIRSIZ], *path;
5606   uint off;
5607 
5608   if(argstr(0, &path) < 0)
5609     return -1;
5610   if((dp = nameiparent(path, name)) == 0)
5611     return -1;
5612 
5613   begin_trans();
5614 
5615   ilock(dp);
5616 
5617   // Cannot unlink "." or "..".
5618   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
5619     goto bad;
5620 
5621   if((ip = dirlookup(dp, name, &off)) == 0)
5622     goto bad;
5623   ilock(ip);
5624 
5625   if(ip->nlink < 1)
5626     panic("unlink: nlink < 1");
5627   if(ip->type == T_DIR && !isdirempty(ip)){
5628     iunlockput(ip);
5629     goto bad;
5630   }
5631 
5632   memset(&de, 0, sizeof(de));
5633   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5634     panic("unlink: writei");
5635   if(ip->type == T_DIR){
5636     dp->nlink--;
5637     iupdate(dp);
5638   }
5639   iunlockput(dp);
5640 
5641   ip->nlink--;
5642   iupdate(ip);
5643   iunlockput(ip);
5644 
5645   commit_trans();
5646 
5647   return 0;
5648 
5649 
5650 bad:
5651   iunlockput(dp);
5652   commit_trans();
5653   return -1;
5654 }
5655 
5656 static struct inode*
5657 create(char *path, short type, short major, short minor)
5658 {
5659   uint off;
5660   struct inode *ip, *dp;
5661   char name[DIRSIZ];
5662 
5663   if((dp = nameiparent(path, name)) == 0)
5664     return 0;
5665   ilock(dp);
5666 
5667   if((ip = dirlookup(dp, name, &off)) != 0){
5668     iunlockput(dp);
5669     ilock(ip);
5670     if(type == T_FILE && ip->type == T_FILE)
5671       return ip;
5672     iunlockput(ip);
5673     return 0;
5674   }
5675 
5676   if((ip = ialloc(dp->dev, type)) == 0)
5677     panic("create: ialloc");
5678 
5679   ilock(ip);
5680   ip->major = major;
5681   ip->minor = minor;
5682   ip->nlink = 1;
5683   iupdate(ip);
5684 
5685   if(type == T_DIR){  // Create . and .. entries.
5686     dp->nlink++;  // for ".."
5687     iupdate(dp);
5688     // No ip->nlink++ for ".": avoid cyclic ref count.
5689     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
5690       panic("create dots");
5691   }
5692 
5693   if(dirlink(dp, name, ip->inum) < 0)
5694     panic("create: dirlink");
5695 
5696   iunlockput(dp);
5697 
5698   return ip;
5699 }
5700 int
5701 sys_open(void)
5702 {
5703   char *path;
5704   int fd, omode;
5705   struct file *f;
5706   struct inode *ip;
5707 
5708   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
5709     return -1;
5710   if(omode & O_CREATE){
5711     begin_trans();
5712     ip = create(path, T_FILE, 0, 0);
5713     commit_trans();
5714     if(ip == 0)
5715       return -1;
5716   } else {
5717     if((ip = namei(path)) == 0)
5718       return -1;
5719     ilock(ip);
5720     if(ip->type == T_DIR && omode != O_RDONLY){
5721       iunlockput(ip);
5722       return -1;
5723     }
5724   }
5725 
5726   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
5727     if(f)
5728       fileclose(f);
5729     iunlockput(ip);
5730     return -1;
5731   }
5732   iunlock(ip);
5733 
5734   f->type = FD_INODE;
5735   f->ip = ip;
5736   f->off = 0;
5737   f->readable = !(omode & O_WRONLY);
5738   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
5739   return fd;
5740 }
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
5750 int
5751 sys_mkdir(void)
5752 {
5753   char *path;
5754   struct inode *ip;
5755 
5756   begin_trans();
5757   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
5758     commit_trans();
5759     return -1;
5760   }
5761   iunlockput(ip);
5762   commit_trans();
5763   return 0;
5764 }
5765 
5766 int
5767 sys_mknod(void)
5768 {
5769   struct inode *ip;
5770   char *path;
5771   int len;
5772   int major, minor;
5773 
5774   begin_trans();
5775   if((len=argstr(0, &path)) < 0 ||
5776      argint(1, &major) < 0 ||
5777      argint(2, &minor) < 0 ||
5778      (ip = create(path, T_DEV, major, minor)) == 0){
5779     commit_trans();
5780     return -1;
5781   }
5782   iunlockput(ip);
5783   commit_trans();
5784   return 0;
5785 }
5786 
5787 
5788 
5789 
5790 
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 int
5801 sys_chdir(void)
5802 {
5803   char *path;
5804   struct inode *ip;
5805 
5806   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0)
5807     return -1;
5808   ilock(ip);
5809   if(ip->type != T_DIR){
5810     iunlockput(ip);
5811     return -1;
5812   }
5813   iunlock(ip);
5814   iput(proc->cwd);
5815   proc->cwd = ip;
5816   return 0;
5817 }
5818 
5819 int
5820 sys_exec(void)
5821 {
5822   char *path, *argv[MAXARG];
5823   int i;
5824   uint uargv, uarg;
5825 
5826   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
5827     return -1;
5828   }
5829   memset(argv, 0, sizeof(argv));
5830   for(i=0;; i++){
5831     if(i >= NELEM(argv))
5832       return -1;
5833     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
5834       return -1;
5835     if(uarg == 0){
5836       argv[i] = 0;
5837       break;
5838     }
5839     if(fetchstr(uarg, &argv[i]) < 0)
5840       return -1;
5841   }
5842   return exec(path, argv);
5843 }
5844 
5845 
5846 
5847 
5848 
5849 
5850 int
5851 sys_pipe(void)
5852 {
5853   int *fd;
5854   struct file *rf, *wf;
5855   int fd0, fd1;
5856 
5857   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
5858     return -1;
5859   if(pipealloc(&rf, &wf) < 0)
5860     return -1;
5861   fd0 = -1;
5862   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
5863     if(fd0 >= 0)
5864       proc->ofile[fd0] = 0;
5865     fileclose(rf);
5866     fileclose(wf);
5867     return -1;
5868   }
5869   fd[0] = fd0;
5870   fd[1] = fd1;
5871   return 0;
5872 }
5873 
5874 
5875 
5876 
5877 
5878 
5879 
5880 
5881 
5882 
5883 
5884 
5885 
5886 
5887 
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
