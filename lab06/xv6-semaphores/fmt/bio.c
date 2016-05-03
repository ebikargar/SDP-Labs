4000 // Buffer cache.
4001 //
4002 // The buffer cache is a linked list of buf structures holding
4003 // cached copies of disk block contents.  Caching disk blocks
4004 // in memory reduces the number of disk reads and also provides
4005 // a synchronization point for disk blocks used by multiple processes.
4006 //
4007 // Interface:
4008 // * To get a buffer for a particular disk block, call bread.
4009 // * After changing buffer data, call bwrite to write it to disk.
4010 // * When done with the buffer, call brelse.
4011 // * Do not use the buffer after calling brelse.
4012 // * Only one process at a time can use a buffer,
4013 //     so do not keep them longer than necessary.
4014 //
4015 // The implementation uses three state flags internally:
4016 // * B_BUSY: the block has been returned from bread
4017 //     and has not been passed back to brelse.
4018 // * B_VALID: the buffer data has been read from the disk.
4019 // * B_DIRTY: the buffer data has been modified
4020 //     and needs to be written to disk.
4021 
4022 #include "types.h"
4023 #include "defs.h"
4024 #include "param.h"
4025 #include "spinlock.h"
4026 #include "buf.h"
4027 
4028 struct {
4029   struct spinlock lock;
4030   struct buf buf[NBUF];
4031 
4032   // Linked list of all buffers, through prev/next.
4033   // head.next is most recently used.
4034   struct buf head;
4035 } bcache;
4036 
4037 void
4038 binit(void)
4039 {
4040   struct buf *b;
4041 
4042   initlock(&bcache.lock, "bcache");
4043 
4044 
4045 
4046 
4047 
4048 
4049 
4050   // Create linked list of buffers
4051   bcache.head.prev = &bcache.head;
4052   bcache.head.next = &bcache.head;
4053   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4054     b->next = bcache.head.next;
4055     b->prev = &bcache.head;
4056     b->dev = -1;
4057     bcache.head.next->prev = b;
4058     bcache.head.next = b;
4059   }
4060 }
4061 
4062 // Look through buffer cache for sector on device dev.
4063 // If not found, allocate fresh block.
4064 // In either case, return B_BUSY buffer.
4065 static struct buf*
4066 bget(uint dev, uint sector)
4067 {
4068   struct buf *b;
4069 
4070   acquire(&bcache.lock);
4071 
4072  loop:
4073   // Is the sector already cached?
4074   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4075     if(b->dev == dev && b->sector == sector){
4076       if(!(b->flags & B_BUSY)){
4077         b->flags |= B_BUSY;
4078         release(&bcache.lock);
4079         return b;
4080       }
4081       sleep(b, &bcache.lock);
4082       goto loop;
4083     }
4084   }
4085 
4086   // Not cached; recycle some non-busy and clean buffer.
4087   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4088     if((b->flags & B_BUSY) == 0 && (b->flags & B_DIRTY) == 0){
4089       b->dev = dev;
4090       b->sector = sector;
4091       b->flags = B_BUSY;
4092       release(&bcache.lock);
4093       return b;
4094     }
4095   }
4096   panic("bget: no buffers");
4097 }
4098 
4099 
4100 // Return a B_BUSY buf with the contents of the indicated disk sector.
4101 struct buf*
4102 bread(uint dev, uint sector)
4103 {
4104   struct buf *b;
4105 
4106   b = bget(dev, sector);
4107   if(!(b->flags & B_VALID))
4108     iderw(b);
4109   return b;
4110 }
4111 
4112 // Write b's contents to disk.  Must be B_BUSY.
4113 void
4114 bwrite(struct buf *b)
4115 {
4116   if((b->flags & B_BUSY) == 0)
4117     panic("bwrite");
4118   b->flags |= B_DIRTY;
4119   iderw(b);
4120 }
4121 
4122 // Release a B_BUSY buffer.
4123 // Move to the head of the MRU list.
4124 void
4125 brelse(struct buf *b)
4126 {
4127   if((b->flags & B_BUSY) == 0)
4128     panic("brelse");
4129 
4130   acquire(&bcache.lock);
4131 
4132   b->next->prev = b->prev;
4133   b->prev->next = b->next;
4134   b->next = bcache.head.next;
4135   b->prev = &bcache.head;
4136   bcache.head.next->prev = b;
4137   bcache.head.next = b;
4138 
4139   b->flags &= ~B_BUSY;
4140   wakeup(b);
4141 
4142   release(&bcache.lock);
4143 }
4144 
4145 
4146 
4147 
4148 
4149 
