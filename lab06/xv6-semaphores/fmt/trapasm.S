3000 #include "mmu.h"
3001 
3002   # vectors.S sends all traps here.
3003 .globl alltraps
3004 alltraps:
3005   # Build trap frame.
3006   pushl %ds
3007   pushl %es
3008   pushl %fs
3009   pushl %gs
3010   pushal
3011 
3012   # Set up data and per-cpu segments.
3013   movw $(SEG_KDATA<<3), %ax
3014   movw %ax, %ds
3015   movw %ax, %es
3016   movw $(SEG_KCPU<<3), %ax
3017   movw %ax, %fs
3018   movw %ax, %gs
3019 
3020   # Call trap(tf), where tf=%esp
3021   pushl %esp
3022   call trap
3023   addl $4, %esp
3024 
3025   # Return falls through to trapret...
3026 .globl trapret
3027 trapret:
3028   popal
3029   popl %gs
3030   popl %fs
3031   popl %es
3032   popl %ds
3033   addl $0x8, %esp  # trapno and errcode
3034   iret
3035 
3036 
3037 
3038 
3039 
3040 
3041 
3042 
3043 
3044 
3045 
3046 
3047 
3048 
3049 
