2750 // Physical memory allocator, intended to allocate
2751 // memory for user processes, kernel stacks, page table pages,
2752 // and pipe buffers. Allocates 4096-byte pages.
2753 
2754 #include "types.h"
2755 #include "defs.h"
2756 #include "param.h"
2757 #include "memlayout.h"
2758 #include "mmu.h"
2759 #include "spinlock.h"
2760 
2761 void freerange(void *vstart, void *vend);
2762 extern char end[]; // first address after kernel loaded from ELF file
2763 
2764 struct run {
2765   struct run *next;
2766 };
2767 
2768 struct {
2769   struct spinlock lock;
2770   int use_lock;
2771   struct run *freelist;
2772 } kmem;
2773 
2774 // Initialization happens in two phases.
2775 // 1. main() calls kinit1() while still using entrypgdir to place just
2776 // the pages mapped by entrypgdir on free list.
2777 // 2. main() calls kinit2() with the rest of the physical pages
2778 // after installing a full page table that maps them on all cores.
2779 void
2780 kinit1(void *vstart, void *vend)
2781 {
2782   initlock(&kmem.lock, "kmem");
2783   kmem.use_lock = 0;
2784   freerange(vstart, vend);
2785 }
2786 
2787 void
2788 kinit2(void *vstart, void *vend)
2789 {
2790   freerange(vstart, vend);
2791   kmem.use_lock = 1;
2792 }
2793 
2794 
2795 
2796 
2797 
2798 
2799 
2800 void
2801 freerange(void *vstart, void *vend)
2802 {
2803   char *p;
2804   p = (char*)PGROUNDUP((uint)vstart);
2805   for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
2806     kfree(p);
2807 }
2808 
2809 
2810 // Free the page of physical memory pointed at by v,
2811 // which normally should have been returned by a
2812 // call to kalloc().  (The exception is when
2813 // initializing the allocator; see kinit above.)
2814 void
2815 kfree(char *v)
2816 {
2817   struct run *r;
2818 
2819   if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
2820     panic("kfree");
2821 
2822   // Fill with junk to catch dangling refs.
2823   memset(v, 1, PGSIZE);
2824 
2825   if(kmem.use_lock)
2826     acquire(&kmem.lock);
2827   r = (struct run*)v;
2828   r->next = kmem.freelist;
2829   kmem.freelist = r;
2830   if(kmem.use_lock)
2831     release(&kmem.lock);
2832 }
2833 
2834 // Allocate one 4096-byte page of physical memory.
2835 // Returns a pointer that the kernel can use.
2836 // Returns 0 if the memory cannot be allocated.
2837 char*
2838 kalloc(void)
2839 {
2840   struct run *r;
2841 
2842   if(kmem.use_lock)
2843     acquire(&kmem.lock);
2844   r = kmem.freelist;
2845   if(r)
2846     kmem.freelist = r->next;
2847   if(kmem.use_lock)
2848     release(&kmem.lock);
2849   return (char*)r;
2850 }
2851 
2852 
2853 
2854 
2855 
2856 
2857 
2858 
2859 
2860 
2861 
2862 
2863 
2864 
2865 
2866 
2867 
2868 
2869 
2870 
2871 
2872 
2873 
2874 
2875 
2876 
2877 
2878 
2879 
2880 
2881 
2882 
2883 
2884 
2885 
2886 
2887 
2888 
2889 
2890 
2891 
2892 
2893 
2894 
2895 
2896 
2897 
2898 
2899 
