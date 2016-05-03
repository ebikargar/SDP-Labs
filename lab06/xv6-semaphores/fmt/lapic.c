6600 // The local APIC manages internal (non-I/O) interrupts.
6601 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
6602 
6603 #include "types.h"
6604 #include "defs.h"
6605 #include "memlayout.h"
6606 #include "traps.h"
6607 #include "mmu.h"
6608 #include "x86.h"
6609 
6610 // Local APIC registers, divided by 4 for use as uint[] indices.
6611 #define ID      (0x0020/4)   // ID
6612 #define VER     (0x0030/4)   // Version
6613 #define TPR     (0x0080/4)   // Task Priority
6614 #define EOI     (0x00B0/4)   // EOI
6615 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
6616   #define ENABLE     0x00000100   // Unit Enable
6617 #define ESR     (0x0280/4)   // Error Status
6618 #define ICRLO   (0x0300/4)   // Interrupt Command
6619   #define INIT       0x00000500   // INIT/RESET
6620   #define STARTUP    0x00000600   // Startup IPI
6621   #define DELIVS     0x00001000   // Delivery status
6622   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
6623   #define DEASSERT   0x00000000
6624   #define LEVEL      0x00008000   // Level triggered
6625   #define BCAST      0x00080000   // Send to all APICs, including self.
6626   #define BUSY       0x00001000
6627   #define FIXED      0x00000000
6628 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
6629 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
6630   #define X1         0x0000000B   // divide counts by 1
6631   #define PERIODIC   0x00020000   // Periodic
6632 #define PCINT   (0x0340/4)   // Performance Counter LVT
6633 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
6634 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
6635 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
6636   #define MASKED     0x00010000   // Interrupt masked
6637 #define TICR    (0x0380/4)   // Timer Initial Count
6638 #define TCCR    (0x0390/4)   // Timer Current Count
6639 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
6640 
6641 volatile uint *lapic;  // Initialized in mp.c
6642 
6643 static void
6644 lapicw(int index, int value)
6645 {
6646   lapic[index] = value;
6647   lapic[ID];  // wait for write to finish, by reading
6648 }
6649 
6650 void
6651 lapicinit(int c)
6652 {
6653   if(!lapic)
6654     return;
6655 
6656   // Enable local APIC; set spurious interrupt vector.
6657   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
6658 
6659   // The timer repeatedly counts down at bus frequency
6660   // from lapic[TICR] and then issues an interrupt.
6661   // If xv6 cared more about precise timekeeping,
6662   // TICR would be calibrated using an external time source.
6663   lapicw(TDCR, X1);
6664   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
6665   lapicw(TICR, 10000000);
6666 
6667   // Disable logical interrupt lines.
6668   lapicw(LINT0, MASKED);
6669   lapicw(LINT1, MASKED);
6670 
6671   // Disable performance counter overflow interrupts
6672   // on machines that provide that interrupt entry.
6673   if(((lapic[VER]>>16) & 0xFF) >= 4)
6674     lapicw(PCINT, MASKED);
6675 
6676   // Map error interrupt to IRQ_ERROR.
6677   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
6678 
6679   // Clear error status register (requires back-to-back writes).
6680   lapicw(ESR, 0);
6681   lapicw(ESR, 0);
6682 
6683   // Ack any outstanding interrupts.
6684   lapicw(EOI, 0);
6685 
6686   // Send an Init Level De-Assert to synchronise arbitration ID's.
6687   lapicw(ICRHI, 0);
6688   lapicw(ICRLO, BCAST | INIT | LEVEL);
6689   while(lapic[ICRLO] & DELIVS)
6690     ;
6691 
6692   // Enable interrupts on the APIC (but not on the processor).
6693   lapicw(TPR, 0);
6694 }
6695 
6696 
6697 
6698 
6699 
6700 int
6701 cpunum(void)
6702 {
6703   // Cannot call cpu when interrupts are enabled:
6704   // result not guaranteed to last long enough to be used!
6705   // Would prefer to panic but even printing is chancy here:
6706   // almost everything, including cprintf and panic, calls cpu,
6707   // often indirectly through acquire and release.
6708   if(readeflags()&FL_IF){
6709     static int n;
6710     if(n++ == 0)
6711       cprintf("cpu called from %x with interrupts enabled\n",
6712         __builtin_return_address(0));
6713   }
6714 
6715   if(lapic)
6716     return lapic[ID]>>24;
6717   return 0;
6718 }
6719 
6720 // Acknowledge interrupt.
6721 void
6722 lapiceoi(void)
6723 {
6724   if(lapic)
6725     lapicw(EOI, 0);
6726 }
6727 
6728 // Spin for a given number of microseconds.
6729 // On real hardware would want to tune this dynamically.
6730 void
6731 microdelay(int us)
6732 {
6733 }
6734 
6735 #define IO_RTC  0x70
6736 
6737 // Start additional processor running entry code at addr.
6738 // See Appendix B of MultiProcessor Specification.
6739 void
6740 lapicstartap(uchar apicid, uint addr)
6741 {
6742   int i;
6743   ushort *wrv;
6744 
6745   // "The BSP must initialize CMOS shutdown code to 0AH
6746   // and the warm reset vector (DWORD based at 40:67) to point at
6747   // the AP startup code prior to the [universal startup algorithm]."
6748   outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
6749   outb(IO_RTC+1, 0x0A);
6750   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
6751   wrv[0] = 0;
6752   wrv[1] = addr >> 4;
6753 
6754   // "Universal startup algorithm."
6755   // Send INIT (level-triggered) interrupt to reset other CPU.
6756   lapicw(ICRHI, apicid<<24);
6757   lapicw(ICRLO, INIT | LEVEL | ASSERT);
6758   microdelay(200);
6759   lapicw(ICRLO, INIT | LEVEL);
6760   microdelay(100);    // should be 10ms, but too slow in Bochs!
6761 
6762   // Send startup IPI (twice!) to enter code.
6763   // Regular hardware is supposed to only accept a STARTUP
6764   // when it is in the halted state due to an INIT.  So the second
6765   // should be ignored, but it is part of the official Intel algorithm.
6766   // Bochs complains about the second one.  Too bad for Bochs.
6767   for(i = 0; i < 2; i++){
6768     lapicw(ICRHI, apicid<<24);
6769     lapicw(ICRLO, STARTUP | (addr>>12));
6770     microdelay(200);
6771   }
6772 }
6773 
6774 
6775 
6776 
6777 
6778 
6779 
6780 
6781 
6782 
6783 
6784 
6785 
6786 
6787 
6788 
6789 
6790 
6791 
6792 
6793 
6794 
6795 
6796 
6797 
6798 
6799 
