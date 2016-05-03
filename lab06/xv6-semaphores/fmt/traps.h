2900 // x86 trap and interrupt constants.
2901 
2902 // Processor-defined:
2903 #define T_DIVIDE         0      // divide error
2904 #define T_DEBUG          1      // debug exception
2905 #define T_NMI            2      // non-maskable interrupt
2906 #define T_BRKPT          3      // breakpoint
2907 #define T_OFLOW          4      // overflow
2908 #define T_BOUND          5      // bounds check
2909 #define T_ILLOP          6      // illegal opcode
2910 #define T_DEVICE         7      // device not available
2911 #define T_DBLFLT         8      // double fault
2912 // #define T_COPROC      9      // reserved (not used since 486)
2913 #define T_TSS           10      // invalid task switch segment
2914 #define T_SEGNP         11      // segment not present
2915 #define T_STACK         12      // stack exception
2916 #define T_GPFLT         13      // general protection fault
2917 #define T_PGFLT         14      // page fault
2918 // #define T_RES        15      // reserved
2919 #define T_FPERR         16      // floating point error
2920 #define T_ALIGN         17      // aligment check
2921 #define T_MCHK          18      // machine check
2922 #define T_SIMDERR       19      // SIMD floating point error
2923 
2924 // These are arbitrarily chosen, but with care not to overlap
2925 // processor defined exceptions or interrupt vectors.
2926 #define T_SYSCALL       64      // system call
2927 #define T_DEFAULT      500      // catchall
2928 
2929 #define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ
2930 
2931 #define IRQ_TIMER        0
2932 #define IRQ_KBD          1
2933 #define IRQ_COM1         4
2934 #define IRQ_IDE         14
2935 #define IRQ_ERROR       19
2936 #define IRQ_SPURIOUS    31
2937 
2938 
2939 
2940 
2941 
2942 
2943 
2944 
2945 
2946 
2947 
2948 
2949 
