3800 // Simple PIO-based (non-DMA) IDE driver code.
3801 
3802 #include "types.h"
3803 #include "defs.h"
3804 #include "param.h"
3805 #include "memlayout.h"
3806 #include "mmu.h"
3807 #include "proc.h"
3808 #include "x86.h"
3809 #include "traps.h"
3810 #include "spinlock.h"
3811 #include "buf.h"
3812 
3813 #define IDE_BSY       0x80
3814 #define IDE_DRDY      0x40
3815 #define IDE_DF        0x20
3816 #define IDE_ERR       0x01
3817 
3818 #define IDE_CMD_READ  0x20
3819 #define IDE_CMD_WRITE 0x30
3820 
3821 // idequeue points to the buf now being read/written to the disk.
3822 // idequeue->qnext points to the next buf to be processed.
3823 // You must hold idelock while manipulating queue.
3824 
3825 static struct spinlock idelock;
3826 static struct buf *idequeue;
3827 
3828 static int havedisk1;
3829 static void idestart(struct buf*);
3830 
3831 // Wait for IDE disk to become ready.
3832 static int
3833 idewait(int checkerr)
3834 {
3835   int r;
3836 
3837   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
3838     ;
3839   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
3840     return -1;
3841   return 0;
3842 }
3843 
3844 
3845 
3846 
3847 
3848 
3849 
3850 void
3851 ideinit(void)
3852 {
3853   int i;
3854 
3855   initlock(&idelock, "ide");
3856   picenable(IRQ_IDE);
3857   ioapicenable(IRQ_IDE, ncpu - 1);
3858   idewait(0);
3859 
3860   // Check if disk 1 is present
3861   outb(0x1f6, 0xe0 | (1<<4));
3862   for(i=0; i<1000; i++){
3863     if(inb(0x1f7) != 0){
3864       havedisk1 = 1;
3865       break;
3866     }
3867   }
3868 
3869   // Switch back to disk 0.
3870   outb(0x1f6, 0xe0 | (0<<4));
3871 }
3872 
3873 // Start the request for b.  Caller must hold idelock.
3874 static void
3875 idestart(struct buf *b)
3876 {
3877   if(b == 0)
3878     panic("idestart");
3879 
3880   idewait(0);
3881   outb(0x3f6, 0);  // generate interrupt
3882   outb(0x1f2, 1);  // number of sectors
3883   outb(0x1f3, b->sector & 0xff);
3884   outb(0x1f4, (b->sector >> 8) & 0xff);
3885   outb(0x1f5, (b->sector >> 16) & 0xff);
3886   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((b->sector>>24)&0x0f));
3887   if(b->flags & B_DIRTY){
3888     outb(0x1f7, IDE_CMD_WRITE);
3889     outsl(0x1f0, b->data, 512/4);
3890   } else {
3891     outb(0x1f7, IDE_CMD_READ);
3892   }
3893 }
3894 
3895 
3896 
3897 
3898 
3899 
3900 // Interrupt handler.
3901 void
3902 ideintr(void)
3903 {
3904   struct buf *b;
3905 
3906   // First queued buffer is the active request.
3907   acquire(&idelock);
3908   if((b = idequeue) == 0){
3909     release(&idelock);
3910     // cprintf("spurious IDE interrupt\n");
3911     return;
3912   }
3913   idequeue = b->qnext;
3914 
3915   // Read data if needed.
3916   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
3917     insl(0x1f0, b->data, 512/4);
3918 
3919   // Wake process waiting for this buf.
3920   b->flags |= B_VALID;
3921   b->flags &= ~B_DIRTY;
3922   wakeup(b);
3923 
3924   // Start disk on next buf in queue.
3925   if(idequeue != 0)
3926     idestart(idequeue);
3927 
3928   release(&idelock);
3929 }
3930 
3931 
3932 
3933 
3934 
3935 
3936 
3937 
3938 
3939 
3940 
3941 
3942 
3943 
3944 
3945 
3946 
3947 
3948 
3949 
3950 // Sync buf with disk.
3951 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
3952 // Else if B_VALID is not set, read buf from disk, set B_VALID.
3953 void
3954 iderw(struct buf *b)
3955 {
3956   struct buf **pp;
3957 
3958   if(!(b->flags & B_BUSY))
3959     panic("iderw: buf not busy");
3960   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
3961     panic("iderw: nothing to do");
3962   if(b->dev != 0 && !havedisk1)
3963     panic("iderw: ide disk 1 not present");
3964 
3965   acquire(&idelock);  //DOC: acquire-lock
3966 
3967   // Append b to idequeue.
3968   b->qnext = 0;
3969   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC: insert-queue
3970     ;
3971   *pp = b;
3972 
3973   // Start disk if necessary.
3974   if(idequeue == b)
3975     idestart(b);
3976 
3977   // Wait for request to finish.
3978   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
3979     sleep(b, &idelock);
3980   }
3981 
3982   release(&idelock);
3983 }
3984 
3985 
3986 
3987 
3988 
3989 
3990 
3991 
3992 
3993 
3994 
3995 
3996 
3997 
3998 
3999 
