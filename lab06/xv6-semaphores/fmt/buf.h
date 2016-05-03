3500 struct buf {
3501   int flags;
3502   uint dev;
3503   uint sector;
3504   struct buf *prev; // LRU cache list
3505   struct buf *next;
3506   struct buf *qnext; // disk queue
3507   uchar data[512];
3508 };
3509 #define B_BUSY  0x1  // buffer is locked by some process
3510 #define B_VALID 0x2  // buffer has been read from disk
3511 #define B_DIRTY 0x4  // buffer needs to be written to disk
3512 
3513 
3514 
3515 
3516 
3517 
3518 
3519 
3520 
3521 
3522 
3523 
3524 
3525 
3526 
3527 
3528 
3529 
3530 
3531 
3532 
3533 
3534 
3535 
3536 
3537 
3538 
3539 
3540 
3541 
3542 
3543 
3544 
3545 
3546 
3547 
3548 
3549 
