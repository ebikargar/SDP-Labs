3650 // On-disk file system format.
3651 // Both the kernel and user programs use this header file.
3652 
3653 // Block 0 is unused.
3654 // Block 1 is super block.
3655 // Blocks 2 through sb.ninodes/IPB hold inodes.
3656 // Then free bitmap blocks holding sb.size bits.
3657 // Then sb.nblocks data blocks.
3658 // Then sb.nlog log blocks.
3659 
3660 #define ROOTINO 1  // root i-number
3661 #define BSIZE 512  // block size
3662 
3663 // File system super block
3664 struct superblock {
3665   uint size;         // Size of file system image (blocks)
3666   uint nblocks;      // Number of data blocks
3667   uint ninodes;      // Number of inodes.
3668   uint nlog;         // Number of log blocks
3669 };
3670 
3671 #define NDIRECT 12
3672 #define NINDIRECT (BSIZE / sizeof(uint))
3673 #define MAXFILE (NDIRECT + NINDIRECT)
3674 
3675 // On-disk inode structure
3676 struct dinode {
3677   short type;           // File type
3678   short major;          // Major device number (T_DEV only)
3679   short minor;          // Minor device number (T_DEV only)
3680   short nlink;          // Number of links to inode in file system
3681   uint size;            // Size of file (bytes)
3682   uint addrs[NDIRECT+1];   // Data block addresses
3683 };
3684 
3685 // Inodes per block.
3686 #define IPB           (BSIZE / sizeof(struct dinode))
3687 
3688 // Block containing inode i
3689 #define IBLOCK(i)     ((i) / IPB + 2)
3690 
3691 // Bitmap bits per block
3692 #define BPB           (BSIZE*8)
3693 
3694 // Block containing bit for block b
3695 #define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)
3696 
3697 // Directory is a file containing a sequence of dirent structures.
3698 #define DIRSIZ 14
3699 
3700 struct dirent {
3701   ushort inum;
3702   char name[DIRSIZ];
3703 };
3704 
3705 
3706 
3707 
3708 
3709 
3710 
3711 
3712 
3713 
3714 
3715 
3716 
3717 
3718 
3719 
3720 
3721 
3722 
3723 
3724 
3725 
3726 
3727 
3728 
3729 
3730 
3731 
3732 
3733 
3734 
3735 
3736 
3737 
3738 
3739 
3740 
3741 
3742 
3743 
3744 
3745 
3746 
3747 
3748 
3749 
