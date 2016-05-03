7700 # Initial process execs /init.
7701 
7702 #include "syscall.h"
7703 #include "traps.h"
7704 
7705 
7706 # exec(init, argv)
7707 .globl start
7708 start:
7709   pushl $argv
7710   pushl $init
7711   pushl $0  // where caller pc would be
7712   movl $SYS_exec, %eax
7713   int $T_SYSCALL
7714 
7715 # for(;;) exit();
7716 exit:
7717   movl $SYS_exit, %eax
7718   int $T_SYSCALL
7719   jmp exit
7720 
7721 # char init[] = "/init\0";
7722 init:
7723   .string "/init\0"
7724 
7725 # char *argv[] = { init, 0 };
7726 .p2align 2
7727 argv:
7728   .long init
7729   .long 0
7730 
7731 
7732 
7733 
7734 
7735 
7736 
7737 
7738 
7739 
7740 
7741 
7742 
7743 
7744 
7745 
7746 
7747 
7748 
7749 
