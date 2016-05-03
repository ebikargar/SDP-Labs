7550 // Intel 8253/8254/82C54 Programmable Interval Timer (PIT).
7551 // Only used on uniprocessors;
7552 // SMP machines use the local APIC timer.
7553 
7554 #include "types.h"
7555 #include "defs.h"
7556 #include "traps.h"
7557 #include "x86.h"
7558 
7559 #define IO_TIMER1       0x040           // 8253 Timer #1
7560 
7561 // Frequency of all three count-down timers;
7562 // (TIMER_FREQ/freq) is the appropriate count
7563 // to generate a frequency of freq Hz.
7564 
7565 #define TIMER_FREQ      1193182
7566 #define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))
7567 
7568 #define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
7569 #define TIMER_SEL0      0x00    // select counter 0
7570 #define TIMER_RATEGEN   0x04    // mode 2, rate generator
7571 #define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
7572 
7573 void
7574 timerinit(void)
7575 {
7576   // Interrupt 100 times/sec.
7577   outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
7578   outb(IO_TIMER1, TIMER_DIV(100) % 256);
7579   outb(IO_TIMER1, TIMER_DIV(100) / 256);
7580   picenable(IRQ_TIMER);
7581 }
7582 
7583 
7584 
7585 
7586 
7587 
7588 
7589 
7590 
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
