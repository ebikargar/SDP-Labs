3750 struct file {
3751   enum { FD_NONE, FD_PIPE, FD_INODE } type;
3752   int ref; // reference count
3753   char readable;
3754   char writable;
3755   struct pipe *pipe;
3756   struct inode *ip;
3757   uint off;
3758 };
3759 
3760 
3761 // in-memory copy of an inode
3762 struct inode {
3763   uint dev;           // Device number
3764   uint inum;          // Inode number
3765   int ref;            // Reference count
3766   int flags;          // I_BUSY, I_VALID
3767 
3768   short type;         // copy of disk inode
3769   short major;
3770   short minor;
3771   short nlink;
3772   uint size;
3773   uint addrs[NDIRECT+1];
3774 };
3775 #define I_BUSY 0x1
3776 #define I_VALID 0x2
3777 
3778 // table mapping major device number to
3779 // device functions
3780 struct devsw {
3781   int (*read)(struct inode*, char*, int);
3782   int (*write)(struct inode*, char*, int);
3783 };
3784 
3785 extern struct devsw devsw[];
3786 
3787 #define CONSOLE 1
3788 
3789 
3790 
3791 
3792 
3793 
3794 
3795 
3796 
3797 
3798 
3799 
