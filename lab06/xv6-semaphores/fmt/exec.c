5900 #include "types.h"
5901 #include "param.h"
5902 #include "memlayout.h"
5903 #include "mmu.h"
5904 #include "proc.h"
5905 #include "defs.h"
5906 #include "x86.h"
5907 #include "elf.h"
5908 
5909 int
5910 exec(char *path, char **argv)
5911 {
5912   char *s, *last;
5913   int i, off;
5914   uint argc, sz, sp, ustack[3+MAXARG+1];
5915   struct elfhdr elf;
5916   struct inode *ip;
5917   struct proghdr ph;
5918   pde_t *pgdir, *oldpgdir;
5919 
5920   if((ip = namei(path)) == 0)
5921     return -1;
5922   ilock(ip);
5923   pgdir = 0;
5924 
5925   // Check ELF header
5926   if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
5927     goto bad;
5928   if(elf.magic != ELF_MAGIC)
5929     goto bad;
5930 
5931   if((pgdir = setupkvm(kalloc)) == 0)
5932     goto bad;
5933 
5934   // Load program into memory.
5935   sz = 0;
5936   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
5937     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
5938       goto bad;
5939     if(ph.type != ELF_PROG_LOAD)
5940       continue;
5941     if(ph.memsz < ph.filesz)
5942       goto bad;
5943     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
5944       goto bad;
5945     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
5946       goto bad;
5947   }
5948   iunlockput(ip);
5949   ip = 0;
5950   // Allocate two pages at the next page boundary.
5951   // Make the first inaccessible.  Use the second as the user stack.
5952   sz = PGROUNDUP(sz);
5953   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
5954     goto bad;
5955   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
5956   sp = sz;
5957 
5958   // Push argument strings, prepare rest of stack in ustack.
5959   for(argc = 0; argv[argc]; argc++) {
5960     if(argc >= MAXARG)
5961       goto bad;
5962     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
5963     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
5964       goto bad;
5965     ustack[3+argc] = sp;
5966   }
5967   ustack[3+argc] = 0;
5968 
5969   ustack[0] = 0xffffffff;  // fake return PC
5970   ustack[1] = argc;
5971   ustack[2] = sp - (argc+1)*4;  // argv pointer
5972 
5973   sp -= (3+argc+1) * 4;
5974   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
5975     goto bad;
5976 
5977   // Save program name for debugging.
5978   for(last=s=path; *s; s++)
5979     if(*s == '/')
5980       last = s+1;
5981   safestrcpy(proc->name, last, sizeof(proc->name));
5982 
5983   // Commit to the user image.
5984   oldpgdir = proc->pgdir;
5985   proc->pgdir = pgdir;
5986   proc->sz = sz;
5987   proc->tf->eip = elf.entry;  // main
5988   proc->tf->esp = sp;
5989   switchuvm(proc);
5990   freevm(oldpgdir);
5991   return 0;
5992 
5993  bad:
5994   if(pgdir)
5995     freevm(pgdir);
5996   if(ip)
5997     iunlockput(ip);
5998   return -1;
5999 }
