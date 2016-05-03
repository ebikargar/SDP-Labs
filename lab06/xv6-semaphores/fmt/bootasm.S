8400 #include "asm.h"
8401 #include "memlayout.h"
8402 #include "mmu.h"
8403 
8404 # Start the first CPU: switch to 32-bit protected mode, jump into C.
8405 # The BIOS loads this code from the first sector of the hard disk into
8406 # memory at physical address 0x7c00 and starts executing in real mode
8407 # with %cs=0 %ip=7c00.
8408 
8409 .code16                       # Assemble for 16-bit mode
8410 .globl start
8411 start:
8412   cli                         # BIOS enabled interrupts; disable
8413 
8414   # Zero data segment registers DS, ES, and SS.
8415   xorw    %ax,%ax             # Set %ax to zero
8416   movw    %ax,%ds             # -> Data Segment
8417   movw    %ax,%es             # -> Extra Segment
8418   movw    %ax,%ss             # -> Stack Segment
8419 
8420   # Physical address line A20 is tied to zero so that the first PCs
8421   # with 2 MB would run software that assumed 1 MB.  Undo that.
8422 seta20.1:
8423   inb     $0x64,%al               # Wait for not busy
8424   testb   $0x2,%al
8425   jnz     seta20.1
8426 
8427   movb    $0xd1,%al               # 0xd1 -> port 0x64
8428   outb    %al,$0x64
8429 
8430 seta20.2:
8431   inb     $0x64,%al               # Wait for not busy
8432   testb   $0x2,%al
8433   jnz     seta20.2
8434 
8435   movb    $0xdf,%al               # 0xdf -> port 0x60
8436   outb    %al,$0x60
8437 
8438   # Switch from real to protected mode.  Use a bootstrap GDT that makes
8439   # virtual addresses map directly to physical addresses so that the
8440   # effective memory map doesn't change during the transition.
8441   lgdt    gdtdesc
8442   movl    %cr0, %eax
8443   orl     $CR0_PE, %eax
8444   movl    %eax, %cr0
8445 
8446 
8447 
8448 
8449 
8450   # Complete transition to 32-bit protected mode by using long jmp
8451   # to reload %cs and %eip.  The segment descriptors are set up with no
8452   # translation, so that the mapping is still the identity mapping.
8453   ljmp    $(SEG_KCODE<<3), $start32
8454 
8455 .code32  # Tell assembler to generate 32-bit code now.
8456 start32:
8457   # Set up the protected-mode data segment registers
8458   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
8459   movw    %ax, %ds                # -> DS: Data Segment
8460   movw    %ax, %es                # -> ES: Extra Segment
8461   movw    %ax, %ss                # -> SS: Stack Segment
8462   movw    $0, %ax                 # Zero segments not ready for use
8463   movw    %ax, %fs                # -> FS
8464   movw    %ax, %gs                # -> GS
8465 
8466   # Set up the stack pointer and call into C.
8467   movl    $start, %esp
8468   call    bootmain
8469 
8470   # If bootmain returns (it shouldn't), trigger a Bochs
8471   # breakpoint if running under Bochs, then loop.
8472   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
8473   movw    %ax, %dx
8474   outw    %ax, %dx
8475   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
8476   outw    %ax, %dx
8477 spin:
8478   jmp     spin
8479 
8480 # Bootstrap GDT
8481 .p2align 2                                # force 4 byte alignment
8482 gdt:
8483   SEG_NULLASM                             # null seg
8484   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
8485   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
8486 
8487 gdtdesc:
8488   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
8489   .long   gdt                             # address gdt
8490 
8491 
8492 
8493 
8494 
8495 
8496 
8497 
8498 
8499 
