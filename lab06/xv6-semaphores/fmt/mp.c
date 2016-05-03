6400 // Multiprocessor support
6401 // Search memory for MP description structures.
6402 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
6403 
6404 #include "types.h"
6405 #include "defs.h"
6406 #include "param.h"
6407 #include "memlayout.h"
6408 #include "mp.h"
6409 #include "x86.h"
6410 #include "mmu.h"
6411 #include "proc.h"
6412 
6413 struct cpu cpus[NCPU];
6414 static struct cpu *bcpu;
6415 int ismp;
6416 int ncpu;
6417 uchar ioapicid;
6418 
6419 int
6420 mpbcpu(void)
6421 {
6422   return bcpu-cpus;
6423 }
6424 
6425 static uchar
6426 sum(uchar *addr, int len)
6427 {
6428   int i, sum;
6429 
6430   sum = 0;
6431   for(i=0; i<len; i++)
6432     sum += addr[i];
6433   return sum;
6434 }
6435 
6436 // Look for an MP structure in the len bytes at addr.
6437 static struct mp*
6438 mpsearch1(uint a, int len)
6439 {
6440   uchar *e, *p, *addr;
6441 
6442   addr = p2v(a);
6443   e = addr+len;
6444   for(p = addr; p < e; p += sizeof(struct mp))
6445     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
6446       return (struct mp*)p;
6447   return 0;
6448 }
6449 
6450 // Search for the MP Floating Pointer Structure, which according to the
6451 // spec is in one of the following three locations:
6452 // 1) in the first KB of the EBDA;
6453 // 2) in the last KB of system base memory;
6454 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
6455 static struct mp*
6456 mpsearch(void)
6457 {
6458   uchar *bda;
6459   uint p;
6460   struct mp *mp;
6461 
6462   bda = (uchar *) P2V(0x400);
6463   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
6464     if((mp = mpsearch1(p, 1024)))
6465       return mp;
6466   } else {
6467     p = ((bda[0x14]<<8)|bda[0x13])*1024;
6468     if((mp = mpsearch1(p-1024, 1024)))
6469       return mp;
6470   }
6471   return mpsearch1(0xF0000, 0x10000);
6472 }
6473 
6474 // Search for an MP configuration table.  For now,
6475 // don't accept the default configurations (physaddr == 0).
6476 // Check for correct signature, calculate the checksum and,
6477 // if correct, check the version.
6478 // To do: check extended table checksum.
6479 static struct mpconf*
6480 mpconfig(struct mp **pmp)
6481 {
6482   struct mpconf *conf;
6483   struct mp *mp;
6484 
6485   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
6486     return 0;
6487   conf = (struct mpconf*) p2v((uint) mp->physaddr);
6488   if(memcmp(conf, "PCMP", 4) != 0)
6489     return 0;
6490   if(conf->version != 1 && conf->version != 4)
6491     return 0;
6492   if(sum((uchar*)conf, conf->length) != 0)
6493     return 0;
6494   *pmp = mp;
6495   return conf;
6496 }
6497 
6498 
6499 
6500 void
6501 mpinit(void)
6502 {
6503   uchar *p, *e;
6504   struct mp *mp;
6505   struct mpconf *conf;
6506   struct mpproc *proc;
6507   struct mpioapic *ioapic;
6508 
6509   bcpu = &cpus[0];
6510   if((conf = mpconfig(&mp)) == 0)
6511     return;
6512   ismp = 1;
6513   lapic = (uint*)conf->lapicaddr;
6514   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
6515     switch(*p){
6516     case MPPROC:
6517       proc = (struct mpproc*)p;
6518       if(ncpu != proc->apicid){
6519         cprintf("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
6520         ismp = 0;
6521       }
6522       if(proc->flags & MPBOOT)
6523         bcpu = &cpus[ncpu];
6524       cpus[ncpu].id = ncpu;
6525       ncpu++;
6526       p += sizeof(struct mpproc);
6527       continue;
6528     case MPIOAPIC:
6529       ioapic = (struct mpioapic*)p;
6530       ioapicid = ioapic->apicno;
6531       p += sizeof(struct mpioapic);
6532       continue;
6533     case MPBUS:
6534     case MPIOINTR:
6535     case MPLINTR:
6536       p += 8;
6537       continue;
6538     default:
6539       cprintf("mpinit: unknown config type %x\n", *p);
6540       ismp = 0;
6541     }
6542   }
6543   if(!ismp){
6544     // Didn't like what we found; fall back to no MP.
6545     ncpu = 1;
6546     lapic = 0;
6547     ioapicid = 0;
6548     return;
6549   }
6550   if(mp->imcrp){
6551     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
6552     // But it would on real hardware.
6553     outb(0x22, 0x70);   // Select IMCR
6554     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
6555   }
6556 }
6557 
6558 
6559 
6560 
6561 
6562 
6563 
6564 
6565 
6566 
6567 
6568 
6569 
6570 
6571 
6572 
6573 
6574 
6575 
6576 
6577 
6578 
6579 
6580 
6581 
6582 
6583 
6584 
6585 
6586 
6587 
6588 
6589 
6590 
6591 
6592 
6593 
6594 
6595 
6596 
6597 
6598 
6599 
