7600 // Intel 8250 serial port (UART).
7601 
7602 #include "types.h"
7603 #include "defs.h"
7604 #include "param.h"
7605 #include "traps.h"
7606 #include "spinlock.h"
7607 #include "fs.h"
7608 #include "file.h"
7609 #include "mmu.h"
7610 #include "proc.h"
7611 #include "x86.h"
7612 
7613 #define COM1    0x3f8
7614 
7615 static int uart;    // is there a uart?
7616 
7617 void
7618 uartinit(void)
7619 {
7620   char *p;
7621 
7622   // Turn off the FIFO
7623   outb(COM1+2, 0);
7624 
7625   // 9600 baud, 8 data bits, 1 stop bit, parity off.
7626   outb(COM1+3, 0x80);    // Unlock divisor
7627   outb(COM1+0, 115200/9600);
7628   outb(COM1+1, 0);
7629   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
7630   outb(COM1+4, 0);
7631   outb(COM1+1, 0x01);    // Enable receive interrupts.
7632 
7633   // If status is 0xFF, no serial port.
7634   if(inb(COM1+5) == 0xFF)
7635     return;
7636   uart = 1;
7637 
7638   // Acknowledge pre-existing interrupt conditions;
7639   // enable interrupts.
7640   inb(COM1+2);
7641   inb(COM1+0);
7642   picenable(IRQ_COM1);
7643   ioapicenable(IRQ_COM1, 0);
7644 
7645   // Announce that we're here.
7646   for(p="xv6...\n"; *p; p++)
7647     uartputc(*p);
7648 }
7649 
7650 void
7651 uartputc(int c)
7652 {
7653   int i;
7654 
7655   if(!uart)
7656     return;
7657   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
7658     microdelay(10);
7659   outb(COM1+0, c);
7660 }
7661 
7662 static int
7663 uartgetc(void)
7664 {
7665   if(!uart)
7666     return -1;
7667   if(!(inb(COM1+5) & 0x01))
7668     return -1;
7669   return inb(COM1+0);
7670 }
7671 
7672 void
7673 uartintr(void)
7674 {
7675   consoleintr(uartgetc);
7676 }
7677 
7678 
7679 
7680 
7681 
7682 
7683 
7684 
7685 
7686 
7687 
7688 
7689 
7690 
7691 
7692 
7693 
7694 
7695 
7696 
7697 
7698 
7699 
