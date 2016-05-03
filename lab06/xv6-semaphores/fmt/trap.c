3050 #include "types.h"
3051 #include "defs.h"
3052 #include "param.h"
3053 #include "memlayout.h"
3054 #include "mmu.h"
3055 #include "proc.h"
3056 #include "x86.h"
3057 #include "traps.h"
3058 #include "spinlock.h"
3059 
3060 // Interrupt descriptor table (shared by all CPUs).
3061 struct gatedesc idt[256];
3062 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3063 struct spinlock tickslock;
3064 uint ticks;
3065 
3066 void
3067 tvinit(void)
3068 {
3069   int i;
3070 
3071   for(i = 0; i < 256; i++)
3072     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3073   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3074 
3075   initlock(&tickslock, "time");
3076 }
3077 
3078 void
3079 idtinit(void)
3080 {
3081   lidt(idt, sizeof(idt));
3082 }
3083 
3084 
3085 
3086 
3087 
3088 
3089 
3090 
3091 
3092 
3093 
3094 
3095 
3096 
3097 
3098 
3099 
3100 void
3101 trap(struct trapframe *tf)
3102 {
3103   if(tf->trapno == T_SYSCALL){
3104     if(proc->killed)
3105       exit();
3106     proc->tf = tf;
3107     syscall();
3108     if(proc->killed)
3109       exit();
3110     return;
3111   }
3112 
3113   switch(tf->trapno){
3114   case T_IRQ0 + IRQ_TIMER:
3115     if(cpu->id == 0){
3116       acquire(&tickslock);
3117       ticks++;
3118       wakeup(&ticks);
3119       release(&tickslock);
3120     }
3121     lapiceoi();
3122     break;
3123   case T_IRQ0 + IRQ_IDE:
3124     ideintr();
3125     lapiceoi();
3126     break;
3127   case T_IRQ0 + IRQ_IDE+1:
3128     // Bochs generates spurious IDE1 interrupts.
3129     break;
3130   case T_IRQ0 + IRQ_KBD:
3131     kbdintr();
3132     lapiceoi();
3133     break;
3134   case T_IRQ0 + IRQ_COM1:
3135     uartintr();
3136     lapiceoi();
3137     break;
3138   case T_IRQ0 + 7:
3139   case T_IRQ0 + IRQ_SPURIOUS:
3140     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3141             cpu->id, tf->cs, tf->eip);
3142     lapiceoi();
3143     break;
3144 
3145 
3146 
3147 
3148 
3149 
3150   default:
3151     if(proc == 0 || (tf->cs&3) == 0){
3152       // In kernel, it must be our mistake.
3153       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3154               tf->trapno, cpu->id, tf->eip, rcr2());
3155       panic("trap");
3156     }
3157     // In user space, assume process misbehaved.
3158     cprintf("pid %d %s: trap %d err %d on cpu %d "
3159             "eip 0x%x addr 0x%x--kill proc\n",
3160             proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip,
3161             rcr2());
3162     proc->killed = 1;
3163   }
3164 
3165   // Force process exit if it has been killed and is in user space.
3166   // (If it is still executing in the kernel, let it keep running
3167   // until it gets to the regular system call return.)
3168   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
3169     exit();
3170 
3171   // Force process to give up CPU on clock tick.
3172   // If interrupts were on while locks held, would need to check nlock.
3173   if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
3174     yield();
3175 
3176   // Check if the process has been killed since we yielded
3177   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
3178     exit();
3179 }
3180 
3181 
3182 
3183 
3184 
3185 
3186 
3187 
3188 
3189 
3190 
3191 
3192 
3193 
3194 
3195 
3196 
3197 
3198 
3199 
