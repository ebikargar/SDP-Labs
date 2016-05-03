2700 # Context switch
2701 #
2702 #   void swtch(struct context **old, struct context *new);
2703 #
2704 # Save current register context in old
2705 # and then load register context from new.
2706 
2707 .globl swtch
2708 swtch:
2709   movl 4(%esp), %eax
2710   movl 8(%esp), %edx
2711 
2712   # Save old callee-save registers
2713   pushl %ebp
2714   pushl %ebx
2715   pushl %esi
2716   pushl %edi
2717 
2718   # Switch stacks
2719   movl %esp, (%eax)
2720   movl %edx, %esp
2721 
2722   # Load new callee-save registers
2723   popl %edi
2724   popl %esi
2725   popl %ebx
2726   popl %ebp
2727   ret
2728 
2729 
2730 
2731 
2732 
2733 
2734 
2735 
2736 
2737 
2738 
2739 
2740 
2741 
2742 
2743 
2744 
2745 
2746 
2747 
2748 
2749 
