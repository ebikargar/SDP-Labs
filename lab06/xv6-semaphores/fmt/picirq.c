6900 // Intel 8259A programmable interrupt controllers.
6901 
6902 #include "types.h"
6903 #include "x86.h"
6904 #include "traps.h"
6905 
6906 // I/O Addresses of the two programmable interrupt controllers
6907 #define IO_PIC1         0x20    // Master (IRQs 0-7)
6908 #define IO_PIC2         0xA0    // Slave (IRQs 8-15)
6909 
6910 #define IRQ_SLAVE       2       // IRQ at which slave connects to master
6911 
6912 // Current IRQ mask.
6913 // Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
6914 static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
6915 
6916 static void
6917 picsetmask(ushort mask)
6918 {
6919   irqmask = mask;
6920   outb(IO_PIC1+1, mask);
6921   outb(IO_PIC2+1, mask >> 8);
6922 }
6923 
6924 void
6925 picenable(int irq)
6926 {
6927   picsetmask(irqmask & ~(1<<irq));
6928 }
6929 
6930 // Initialize the 8259A interrupt controllers.
6931 void
6932 picinit(void)
6933 {
6934   // mask all interrupts
6935   outb(IO_PIC1+1, 0xFF);
6936   outb(IO_PIC2+1, 0xFF);
6937 
6938   // Set up master (8259A-1)
6939 
6940   // ICW1:  0001g0hi
6941   //    g:  0 = edge triggering, 1 = level triggering
6942   //    h:  0 = cascaded PICs, 1 = master only
6943   //    i:  0 = no ICW4, 1 = ICW4 required
6944   outb(IO_PIC1, 0x11);
6945 
6946   // ICW2:  Vector offset
6947   outb(IO_PIC1+1, T_IRQ0);
6948 
6949 
6950   // ICW3:  (master PIC) bit mask of IR lines connected to slaves
6951   //        (slave PIC) 3-bit # of slave's connection to master
6952   outb(IO_PIC1+1, 1<<IRQ_SLAVE);
6953 
6954   // ICW4:  000nbmap
6955   //    n:  1 = special fully nested mode
6956   //    b:  1 = buffered mode
6957   //    m:  0 = slave PIC, 1 = master PIC
6958   //      (ignored when b is 0, as the master/slave role
6959   //      can be hardwired).
6960   //    a:  1 = Automatic EOI mode
6961   //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
6962   outb(IO_PIC1+1, 0x3);
6963 
6964   // Set up slave (8259A-2)
6965   outb(IO_PIC2, 0x11);                  // ICW1
6966   outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
6967   outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
6968   // NB Automatic EOI mode doesn't tend to work on the slave.
6969   // Linux source code says it's "to be investigated".
6970   outb(IO_PIC2+1, 0x3);                 // ICW4
6971 
6972   // OCW3:  0ef01prs
6973   //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
6974   //    p:  0 = no polling, 1 = polling mode
6975   //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
6976   outb(IO_PIC1, 0x68);             // clear specific mask
6977   outb(IO_PIC1, 0x0a);             // read IRR by default
6978 
6979   outb(IO_PIC2, 0x68);             // OCW3
6980   outb(IO_PIC2, 0x0a);             // OCW3
6981 
6982   if(irqmask != 0xFFFF)
6983     picsetmask(irqmask);
6984 }
6985 
6986 
6987 
6988 
6989 
6990 
6991 
6992 
6993 
6994 
6995 
6996 
6997 
6998 
6999 
