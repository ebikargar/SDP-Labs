0250 struct buf;
0251 struct context;
0252 struct file;
0253 struct inode;
0254 struct pipe;
0255 struct proc;
0256 struct spinlock;
0257 struct stat;
0258 struct superblock;
0259 
0260 // bio.c
0261 void            binit(void);
0262 struct buf*     bread(uint, uint);
0263 void            brelse(struct buf*);
0264 void            bwrite(struct buf*);
0265 
0266 // console.c
0267 void            consoleinit(void);
0268 void            cprintf(char*, ...);
0269 void            consoleintr(int(*)(void));
0270 void            panic(char*) __attribute__((noreturn));
0271 
0272 // exec.c
0273 int             exec(char*, char**);
0274 
0275 // file.c
0276 struct file*    filealloc(void);
0277 void            fileclose(struct file*);
0278 struct file*    filedup(struct file*);
0279 void            fileinit(void);
0280 int             fileread(struct file*, char*, int n);
0281 int             filestat(struct file*, struct stat*);
0282 int             filewrite(struct file*, char*, int n);
0283 
0284 // fs.c
0285 void            readsb(int dev, struct superblock *sb);
0286 int             dirlink(struct inode*, char*, uint);
0287 struct inode*   dirlookup(struct inode*, char*, uint*);
0288 struct inode*   ialloc(uint, short);
0289 struct inode*   idup(struct inode*);
0290 void            iinit(void);
0291 void            ilock(struct inode*);
0292 void            iput(struct inode*);
0293 void            iunlock(struct inode*);
0294 void            iunlockput(struct inode*);
0295 void            iupdate(struct inode*);
0296 int             namecmp(const char*, const char*);
0297 struct inode*   namei(char*);
0298 struct inode*   nameiparent(char*, char*);
0299 int             readi(struct inode*, char*, uint, uint);
0300 void            stati(struct inode*, struct stat*);
0301 int             writei(struct inode*, char*, uint, uint);
0302 
0303 // ide.c
0304 void            ideinit(void);
0305 void            ideintr(void);
0306 void            iderw(struct buf*);
0307 
0308 // ioapic.c
0309 void            ioapicenable(int irq, int cpu);
0310 extern uchar    ioapicid;
0311 void            ioapicinit(void);
0312 
0313 // kalloc.c
0314 char*           kalloc(void);
0315 void            kfree(char*);
0316 void            kinit1(void*, void*);
0317 void            kinit2(void*, void*);
0318 
0319 // kbd.c
0320 void            kbdintr(void);
0321 
0322 // lapic.c
0323 int             cpunum(void);
0324 extern volatile uint*    lapic;
0325 void            lapiceoi(void);
0326 void            lapicinit(int);
0327 void            lapicstartap(uchar, uint);
0328 void            microdelay(int);
0329 
0330 // log.c
0331 void            initlog(void);
0332 void            log_write(struct buf*);
0333 void            begin_trans();
0334 void            commit_trans();
0335 
0336 // mp.c
0337 extern int      ismp;
0338 int             mpbcpu(void);
0339 void            mpinit(void);
0340 void            mpstartthem(void);
0341 
0342 // picirq.c
0343 void            picenable(int);
0344 void            picinit(void);
0345 
0346 
0347 
0348 
0349 
0350 // pipe.c
0351 int             pipealloc(struct file**, struct file**);
0352 void            pipeclose(struct pipe*, int);
0353 int             piperead(struct pipe*, char*, int);
0354 int             pipewrite(struct pipe*, char*, int);
0355 
0356 
0357 // proc.c
0358 struct proc*    copyproc(struct proc*);
0359 void            exit(void);
0360 int             fork(void);
0361 int             growproc(int);
0362 int             kill(int);
0363 void            pinit(void);
0364 void            procdump(void);
0365 void            scheduler(void) __attribute__((noreturn));
0366 void            sched(void);
0367 void            sleep(void*, struct spinlock*);
0368 void            userinit(void);
0369 int             wait(void);
0370 void            wakeup(void*);
0371 void            yield(void);
0372 
0373 // swtch.S
0374 void            swtch(struct context**, struct context*);
0375 
0376 // spinlock.c
0377 void            acquire(struct spinlock*);
0378 void            getcallerpcs(void*, uint*);
0379 int             holding(struct spinlock*);
0380 void            initlock(struct spinlock*, char*);
0381 void            release(struct spinlock*);
0382 void            pushcli(void);
0383 void            popcli(void);
0384 
0385 // string.c
0386 int             memcmp(const void*, const void*, uint);
0387 void*           memmove(void*, const void*, uint);
0388 void*           memset(void*, int, uint);
0389 char*           safestrcpy(char*, const char*, int);
0390 int             strlen(const char*);
0391 int             strncmp(const char*, const char*, uint);
0392 char*           strncpy(char*, const char*, int);
0393 
0394 // syscall.c
0395 int             argint(int, int*);
0396 int             argptr(int, char**, int);
0397 int             argstr(int, char**);
0398 int             fetchint(uint, int*);
0399 int             fetchstr(uint, char**);
0400 void            syscall(void);
0401 
0402 // timer.c
0403 void            timerinit(void);
0404 
0405 // trap.c
0406 void            idtinit(void);
0407 extern uint     ticks;
0408 void            tvinit(void);
0409 extern struct spinlock tickslock;
0410 
0411 // uart.c
0412 void            uartinit(void);
0413 void            uartintr(void);
0414 void            uartputc(int);
0415 
0416 // vm.c
0417 void            seginit(void);
0418 void            kvmalloc(void);
0419 void            vmenable(void);
0420 pde_t*          setupkvm();
0421 char*           uva2ka(pde_t*, char*);
0422 int             allocuvm(pde_t*, uint, uint);
0423 int             deallocuvm(pde_t*, uint, uint);
0424 void            freevm(pde_t*);
0425 void            inituvm(pde_t*, char*, uint);
0426 int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
0427 pde_t*          copyuvm(pde_t*, uint);
0428 void            switchuvm(struct proc*);
0429 void            switchkvm(void);
0430 int             copyout(pde_t*, uint, void*, uint);
0431 void            clearpteu(pde_t *pgdir, char *uva);
0432 
0433 // number of elements in fixed-size array
0434 #define NELEM(x) (sizeof(x)/sizeof((x)[0]))
0435 
0436 
0437 
0438 
0439 
0440 
0441 
0442 
0443 
0444 
0445 
0446 
0447 
0448 
0449 
