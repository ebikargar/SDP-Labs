2050 // Segments in proc->gdt.
2051 #define NSEGS     7
2052 
2053 // Per-CPU state
2054 struct cpu {
2055   uchar id;                    // Local APIC ID; index into cpus[] below
2056   struct context *scheduler;   // swtch() here to enter scheduler
2057   struct taskstate ts;         // Used by x86 to find stack for interrupt
2058   struct segdesc gdt[NSEGS];   // x86 global descriptor table
2059   volatile uint started;        // Has the CPU started?
2060   int ncli;                    // Depth of pushcli nesting.
2061   int intena;                  // Were interrupts enabled before pushcli?
2062 
2063   // Cpu-local storage variables; see below
2064   struct cpu *cpu;
2065   struct proc *proc;           // The currently-running process.
2066 };
2067 
2068 extern struct cpu cpus[NCPU];
2069 extern int ncpu;
2070 
2071 // Per-CPU variables, holding pointers to the
2072 // current cpu and to the current process.
2073 // The asm suffix tells gcc to use "%gs:0" to refer to cpu
2074 // and "%gs:4" to refer to proc.  seginit sets up the
2075 // %gs segment register so that %gs refers to the memory
2076 // holding those two variables in the local cpu's struct cpu.
2077 // This is similar to how thread-local variables are implemented
2078 // in thread libraries such as Linux pthreads.
2079 extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
2080 extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
2081 
2082 
2083 // Saved registers for kernel context switches.
2084 // Don't need to save all the segment registers (%cs, etc),
2085 // because they are constant across kernel contexts.
2086 // Don't need to save %eax, %ecx, %edx, because the
2087 // x86 convention is that the caller has saved them.
2088 // Contexts are stored at the bottom of the stack they
2089 // describe; the stack pointer is the address of the context.
2090 // The layout of the context matches the layout of the stack in swtch.S
2091 // at the "Switch stacks" comment. Switch doesn't save eip explicitly,
2092 // but it is on the stack and allocproc() manipulates it.
2093 struct context {
2094   uint edi;
2095   uint esi;
2096   uint ebx;
2097   uint ebp;
2098   uint eip;
2099 };
2100 enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
2101 
2102 // Per-process state
2103 struct proc {
2104   uint sz;                     // Size of process memory (bytes)
2105   pde_t* pgdir;                // Page table
2106   char *kstack;                // Bottom of kernel stack for this process
2107   enum procstate state;        // Process state
2108   volatile int pid;            // Process ID
2109   struct proc *parent;         // Parent process
2110   struct trapframe *tf;        // Trap frame for current syscall
2111   struct context *context;     // swtch() here to run process
2112   void *chan;                  // If non-zero, sleeping on chan
2113   int killed;                  // If non-zero, have been killed
2114   struct file *ofile[NOFILE];  // Open files
2115   struct inode *cwd;           // Current directory
2116   char name[16];               // Process name (debugging)
2117 };
2118 
2119 // Process memory is laid out contiguously, low addresses first:
2120 //   text
2121 //   original data and bss
2122 //   fixed-size stack
2123 //   expandable heap
2124 
2125 
2126 
2127 
2128 
2129 
2130 
2131 
2132 
2133 
2134 
2135 
2136 
2137 
2138 
2139 
2140 
2141 
2142 
2143 
2144 
2145 
2146 
2147 
2148 
2149 
