3400 #include "types.h"
3401 #include "x86.h"
3402 #include "defs.h"
3403 #include "param.h"
3404 #include "memlayout.h"
3405 #include "mmu.h"
3406 #include "proc.h"
3407 
3408 int
3409 sys_fork(void)
3410 {
3411   return fork();
3412 }
3413 
3414 int
3415 sys_exit(void)
3416 {
3417   exit();
3418   return 0;  // not reached
3419 }
3420 
3421 int
3422 sys_wait(void)
3423 {
3424   return wait();
3425 }
3426 
3427 int
3428 sys_kill(void)
3429 {
3430   int pid;
3431 
3432   if(argint(0, &pid) < 0)
3433     return -1;
3434   return kill(pid);
3435 }
3436 
3437 int
3438 sys_getpid(void)
3439 {
3440   return proc->pid;
3441 }
3442 
3443 
3444 
3445 
3446 
3447 
3448 
3449 
3450 int
3451 sys_sbrk(void)
3452 {
3453   int addr;
3454   int n;
3455 
3456   if(argint(0, &n) < 0)
3457     return -1;
3458   addr = proc->sz;
3459   if(growproc(n) < 0)
3460     return -1;
3461   return addr;
3462 }
3463 
3464 int
3465 sys_sleep(void)
3466 {
3467   int n;
3468   uint ticks0;
3469 
3470   if(argint(0, &n) < 0)
3471     return -1;
3472   acquire(&tickslock);
3473   ticks0 = ticks;
3474   while(ticks - ticks0 < n){
3475     if(proc->killed){
3476       release(&tickslock);
3477       return -1;
3478     }
3479     sleep(&ticks, &tickslock);
3480   }
3481   release(&tickslock);
3482   return 0;
3483 }
3484 
3485 // return how many clock tick interrupts have occurred
3486 // since start.
3487 int
3488 sys_uptime(void)
3489 {
3490   uint xticks;
3491 
3492   acquire(&tickslock);
3493   xticks = ticks;
3494   release(&tickslock);
3495   return xticks;
3496 }
3497 
3498 
3499 
