8500 // Boot loader.
8501 //
8502 // Part of the boot sector, along with bootasm.S, which calls bootmain().
8503 // bootasm.S has put the processor into protected 32-bit mode.
8504 // bootmain() loads an ELF kernel image from the disk starting at
8505 // sector 1 and then jumps to the kernel entry routine.
8506 
8507 #include "types.h"
8508 #include "elf.h"
8509 #include "x86.h"
8510 #include "memlayout.h"
8511 
8512 #define SECTSIZE  512
8513 
8514 void readseg(uchar*, uint, uint);
8515 
8516 void
8517 bootmain(void)
8518 {
8519   struct elfhdr *elf;
8520   struct proghdr *ph, *eph;
8521   void (*entry)(void);
8522   uchar* pa;
8523 
8524   elf = (struct elfhdr*)0x10000;  // scratch space
8525 
8526   // Read 1st page off disk
8527   readseg((uchar*)elf, 4096, 0);
8528 
8529   // Is this an ELF executable?
8530   if(elf->magic != ELF_MAGIC)
8531     return;  // let bootasm.S handle error
8532 
8533   // Load each program segment (ignores ph flags).
8534   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
8535   eph = ph + elf->phnum;
8536   for(; ph < eph; ph++){
8537     pa = (uchar*)ph->paddr;
8538     readseg(pa, ph->filesz, ph->off);
8539     if(ph->memsz > ph->filesz)
8540       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
8541   }
8542 
8543   // Call the entry point from the ELF header.
8544   // Does not return!
8545   entry = (void(*)(void))(elf->entry);
8546   entry();
8547 }
8548 
8549 
8550 void
8551 waitdisk(void)
8552 {
8553   // Wait for disk ready.
8554   while((inb(0x1F7) & 0xC0) != 0x40)
8555     ;
8556 }
8557 
8558 // Read a single sector at offset into dst.
8559 void
8560 readsect(void *dst, uint offset)
8561 {
8562   // Issue command.
8563   waitdisk();
8564   outb(0x1F2, 1);   // count = 1
8565   outb(0x1F3, offset);
8566   outb(0x1F4, offset >> 8);
8567   outb(0x1F5, offset >> 16);
8568   outb(0x1F6, (offset >> 24) | 0xE0);
8569   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
8570 
8571   // Read data.
8572   waitdisk();
8573   insl(0x1F0, dst, SECTSIZE/4);
8574 }
8575 
8576 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
8577 // Might copy more than asked.
8578 void
8579 readseg(uchar* pa, uint count, uint offset)
8580 {
8581   uchar* epa;
8582 
8583   epa = pa + count;
8584 
8585   // Round down to sector boundary.
8586   pa -= offset % SECTSIZE;
8587 
8588   // Translate from bytes to sectors; kernel starts at sector 1.
8589   offset = (offset / SECTSIZE) + 1;
8590 
8591   // If this is too slow, we could read lots of sectors at a time.
8592   // We'd write more to memory than asked, but it doesn't matter --
8593   // we load in increasing order.
8594   for(; pa < epa; pa += SECTSIZE, offset++)
8595     readsect(pa, offset);
8596 }
8597 
8598 
8599 
