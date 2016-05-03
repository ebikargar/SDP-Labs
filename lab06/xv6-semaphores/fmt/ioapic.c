6800 // The I/O APIC manages hardware interrupts for an SMP system.
6801 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
6802 // See also picirq.c.
6803 
6804 #include "types.h"
6805 #include "defs.h"
6806 #include "traps.h"
6807 
6808 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
6809 
6810 #define REG_ID     0x00  // Register index: ID
6811 #define REG_VER    0x01  // Register index: version
6812 #define REG_TABLE  0x10  // Redirection table base
6813 
6814 // The redirection table starts at REG_TABLE and uses
6815 // two registers to configure each interrupt.
6816 // The first (low) register in a pair contains configuration bits.
6817 // The second (high) register contains a bitmask telling which
6818 // CPUs can serve that interrupt.
6819 #define INT_DISABLED   0x00010000  // Interrupt disabled
6820 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
6821 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
6822 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
6823 
6824 volatile struct ioapic *ioapic;
6825 
6826 // IO APIC MMIO structure: write reg, then read or write data.
6827 struct ioapic {
6828   uint reg;
6829   uint pad[3];
6830   uint data;
6831 };
6832 
6833 static uint
6834 ioapicread(int reg)
6835 {
6836   ioapic->reg = reg;
6837   return ioapic->data;
6838 }
6839 
6840 static void
6841 ioapicwrite(int reg, uint data)
6842 {
6843   ioapic->reg = reg;
6844   ioapic->data = data;
6845 }
6846 
6847 
6848 
6849 
6850 void
6851 ioapicinit(void)
6852 {
6853   int i, id, maxintr;
6854 
6855   if(!ismp)
6856     return;
6857 
6858   ioapic = (volatile struct ioapic*)IOAPIC;
6859   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
6860   id = ioapicread(REG_ID) >> 24;
6861   if(id != ioapicid)
6862     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
6863 
6864   // Mark all interrupts edge-triggered, active high, disabled,
6865   // and not routed to any CPUs.
6866   for(i = 0; i <= maxintr; i++){
6867     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
6868     ioapicwrite(REG_TABLE+2*i+1, 0);
6869   }
6870 }
6871 
6872 void
6873 ioapicenable(int irq, int cpunum)
6874 {
6875   if(!ismp)
6876     return;
6877 
6878   // Mark interrupt edge-triggered, active high,
6879   // enabled, and routed to the given cpunum,
6880   // which happens to be that cpu's APIC ID.
6881   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
6882   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
6883 }
6884 
6885 
6886 
6887 
6888 
6889 
6890 
6891 
6892 
6893 
6894 
6895 
6896 
6897 
6898 
6899 
