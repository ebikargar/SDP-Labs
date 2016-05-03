2150 #include "types.h"
2151 #include "defs.h"
2152 #include "param.h"
2153 #include "memlayout.h"
2154 #include "mmu.h"
2155 #include "x86.h"
2156 #include "proc.h"
2157 #include "spinlock.h"
2158 
2159 struct {
2160   struct spinlock lock;
2161   struct proc proc[NPROC];
2162 } ptable;
2163 
2164 static struct proc *initproc;
2165 
2166 int nextpid = 1;
2167 extern void forkret(void);
2168 extern void trapret(void);
2169 
2170 static void wakeup1(void *chan);
2171 
2172 void
2173 pinit(void)
2174 {
2175   initlock(&ptable.lock, "ptable");
2176 }
2177 
2178 
2179 
2180 
2181 
2182 
2183 
2184 
2185 
2186 
2187 
2188 
2189 
2190 
2191 
2192 
2193 
2194 
2195 
2196 
2197 
2198 
2199 
2200 // Look in the process table for an UNUSED proc.
2201 // If found, change state to EMBRYO and initialize
2202 // state required to run in the kernel.
2203 // Otherwise return 0.
2204 static struct proc*
2205 allocproc(void)
2206 {
2207   struct proc *p;
2208   char *sp;
2209 
2210   acquire(&ptable.lock);
2211   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2212     if(p->state == UNUSED)
2213       goto found;
2214   release(&ptable.lock);
2215   return 0;
2216 
2217 found:
2218   p->state = EMBRYO;
2219   p->pid = nextpid++;
2220   release(&ptable.lock);
2221 
2222   // Allocate kernel stack.
2223   if((p->kstack = kalloc()) == 0){
2224     p->state = UNUSED;
2225     return 0;
2226   }
2227   sp = p->kstack + KSTACKSIZE;
2228 
2229   // Leave room for trap frame.
2230   sp -= sizeof *p->tf;
2231   p->tf = (struct trapframe*)sp;
2232 
2233   // Set up new context to start executing at forkret,
2234   // which returns to trapret.
2235   sp -= 4;
2236   *(uint*)sp = (uint)trapret;
2237 
2238   sp -= sizeof *p->context;
2239   p->context = (struct context*)sp;
2240   memset(p->context, 0, sizeof *p->context);
2241   p->context->eip = (uint)forkret;
2242 
2243   return p;
2244 }
2245 
2246 
2247 
2248 
2249 
2250 // Set up first user process.
2251 void
2252 userinit(void)
2253 {
2254   struct proc *p;
2255   extern char _binary_initcode_start[], _binary_initcode_size[];
2256 
2257   p = allocproc();
2258   initproc = p;
2259   if((p->pgdir = setupkvm(kalloc)) == 0)
2260     panic("userinit: out of memory?");
2261   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2262   p->sz = PGSIZE;
2263   memset(p->tf, 0, sizeof(*p->tf));
2264   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2265   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2266   p->tf->es = p->tf->ds;
2267   p->tf->ss = p->tf->ds;
2268   p->tf->eflags = FL_IF;
2269   p->tf->esp = PGSIZE;
2270   p->tf->eip = 0;  // beginning of initcode.S
2271 
2272   safestrcpy(p->name, "initcode", sizeof(p->name));
2273   p->cwd = namei("/");
2274 
2275   p->state = RUNNABLE;
2276 }
2277 
2278 // Grow current process's memory by n bytes.
2279 // Return 0 on success, -1 on failure.
2280 int
2281 growproc(int n)
2282 {
2283   uint sz;
2284 
2285   sz = proc->sz;
2286   if(n > 0){
2287     if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
2288       return -1;
2289   } else if(n < 0){
2290     if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
2291       return -1;
2292   }
2293   proc->sz = sz;
2294   switchuvm(proc);
2295   return 0;
2296 }
2297 
2298 
2299 
2300 // Create a new process copying p as the parent.
2301 // Sets up stack to return as if from system call.
2302 // Caller must set state of returned proc to RUNNABLE.
2303 int
2304 fork(void)
2305 {
2306   int i, pid;
2307   struct proc *np;
2308 
2309   // Allocate process.
2310   if((np = allocproc()) == 0)
2311     return -1;
2312 
2313   // Copy process state from p.
2314   if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
2315     kfree(np->kstack);
2316     np->kstack = 0;
2317     np->state = UNUSED;
2318     return -1;
2319   }
2320   np->sz = proc->sz;
2321   np->parent = proc;
2322   *np->tf = *proc->tf;
2323 
2324   // Clear %eax so that fork returns 0 in the child.
2325   np->tf->eax = 0;
2326 
2327   for(i = 0; i < NOFILE; i++)
2328     if(proc->ofile[i])
2329       np->ofile[i] = filedup(proc->ofile[i]);
2330   np->cwd = idup(proc->cwd);
2331 
2332   pid = np->pid;
2333   np->state = RUNNABLE;
2334   safestrcpy(np->name, proc->name, sizeof(proc->name));
2335   return pid;
2336 }
2337 
2338 
2339 
2340 
2341 
2342 
2343 
2344 
2345 
2346 
2347 
2348 
2349 
2350 // Exit the current process.  Does not return.
2351 // An exited process remains in the zombie state
2352 // until its parent calls wait() to find out it exited.
2353 void
2354 exit(void)
2355 {
2356   struct proc *p;
2357   int fd;
2358 
2359   if(proc == initproc)
2360     panic("init exiting");
2361 
2362   // Close all open files.
2363   for(fd = 0; fd < NOFILE; fd++){
2364     if(proc->ofile[fd]){
2365       fileclose(proc->ofile[fd]);
2366       proc->ofile[fd] = 0;
2367     }
2368   }
2369 
2370   iput(proc->cwd);
2371   proc->cwd = 0;
2372 
2373   acquire(&ptable.lock);
2374 
2375   // Parent might be sleeping in wait().
2376   wakeup1(proc->parent);
2377 
2378   // Pass abandoned children to init.
2379   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2380     if(p->parent == proc){
2381       p->parent = initproc;
2382       if(p->state == ZOMBIE)
2383         wakeup1(initproc);
2384     }
2385   }
2386 
2387   // Jump into the scheduler, never to return.
2388   proc->state = ZOMBIE;
2389   sched();
2390   panic("zombie exit");
2391 }
2392 
2393 
2394 
2395 
2396 
2397 
2398 
2399 
2400 // Wait for a child process to exit and return its pid.
2401 // Return -1 if this process has no children.
2402 int
2403 wait(void)
2404 {
2405   struct proc *p;
2406   int havekids, pid;
2407 
2408   acquire(&ptable.lock);
2409   for(;;){
2410     // Scan through table looking for zombie children.
2411     havekids = 0;
2412     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2413       if(p->parent != proc)
2414         continue;
2415       havekids = 1;
2416       if(p->state == ZOMBIE){
2417         // Found one.
2418         pid = p->pid;
2419         kfree(p->kstack);
2420         p->kstack = 0;
2421         freevm(p->pgdir);
2422         p->state = UNUSED;
2423         p->pid = 0;
2424         p->parent = 0;
2425         p->name[0] = 0;
2426         p->killed = 0;
2427         release(&ptable.lock);
2428         return pid;
2429       }
2430     }
2431 
2432     // No point waiting if we don't have any children.
2433     if(!havekids || proc->killed){
2434       release(&ptable.lock);
2435       return -1;
2436     }
2437 
2438     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2439     sleep(proc, &ptable.lock);  //DOC: wait-sleep
2440   }
2441 }
2442 
2443 
2444 
2445 
2446 
2447 
2448 
2449 
2450 // Per-CPU process scheduler.
2451 // Each CPU calls scheduler() after setting itself up.
2452 // Scheduler never returns.  It loops, doing:
2453 //  - choose a process to run
2454 //  - swtch to start running that process
2455 //  - eventually that process transfers control
2456 //      via swtch back to the scheduler.
2457 void
2458 scheduler(void)
2459 {
2460   struct proc *p;
2461 
2462   for(;;){
2463     // Enable interrupts on this processor.
2464     sti();
2465 
2466     // Loop over process table looking for process to run.
2467     acquire(&ptable.lock);
2468     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2469       if(p->state != RUNNABLE)
2470         continue;
2471 
2472       // Switch to chosen process.  It is the process's job
2473       // to release ptable.lock and then reacquire it
2474       // before jumping back to us.
2475       proc = p;
2476       switchuvm(p);
2477       p->state = RUNNING;
2478       swtch(&cpu->scheduler, proc->context);
2479       switchkvm();
2480 
2481       // Process is done running for now.
2482       // It should have changed its p->state before coming back.
2483       proc = 0;
2484     }
2485     release(&ptable.lock);
2486 
2487   }
2488 }
2489 
2490 
2491 
2492 
2493 
2494 
2495 
2496 
2497 
2498 
2499 
2500 // Enter scheduler.  Must hold only ptable.lock
2501 // and have changed proc->state.
2502 void
2503 sched(void)
2504 {
2505   int intena;
2506 
2507   if(!holding(&ptable.lock))
2508     panic("sched ptable.lock");
2509   if(cpu->ncli != 1)
2510     panic("sched locks");
2511   if(proc->state == RUNNING)
2512     panic("sched running");
2513   if(readeflags()&FL_IF)
2514     panic("sched interruptible");
2515   intena = cpu->intena;
2516   swtch(&proc->context, cpu->scheduler);
2517   cpu->intena = intena;
2518 }
2519 
2520 // Give up the CPU for one scheduling round.
2521 void
2522 yield(void)
2523 {
2524   acquire(&ptable.lock);  //DOC: yieldlock
2525   proc->state = RUNNABLE;
2526   sched();
2527   release(&ptable.lock);
2528 }
2529 
2530 // A fork child's very first scheduling by scheduler()
2531 // will swtch here.  "Return" to user space.
2532 void
2533 forkret(void)
2534 {
2535   static int first = 1;
2536   // Still holding ptable.lock from scheduler.
2537   release(&ptable.lock);
2538 
2539   if (first) {
2540     // Some initialization functions must be run in the context
2541     // of a regular process (e.g., they call sleep), and thus cannot
2542     // be run from main().
2543     first = 0;
2544     initlog();
2545   }
2546 
2547   // Return to "caller", actually trapret (see allocproc).
2548 }
2549 
2550 // Atomically release lock and sleep on chan.
2551 // Reacquires lock when awakened.
2552 void
2553 sleep(void *chan, struct spinlock *lk)
2554 {
2555   if(proc == 0)
2556     panic("sleep");
2557 
2558   if(lk == 0)
2559     panic("sleep without lk");
2560 
2561   // Must acquire ptable.lock in order to
2562   // change p->state and then call sched.
2563   // Once we hold ptable.lock, we can be
2564   // guaranteed that we won't miss any wakeup
2565   // (wakeup runs with ptable.lock locked),
2566   // so it's okay to release lk.
2567   if(lk != &ptable.lock){  //DOC: sleeplock0
2568     acquire(&ptable.lock);  //DOC: sleeplock1
2569     release(lk);
2570   }
2571 
2572   // Go to sleep.
2573   proc->chan = chan;
2574   proc->state = SLEEPING;
2575   sched();
2576 
2577   // Tidy up.
2578   proc->chan = 0;
2579 
2580   // Reacquire original lock.
2581   if(lk != &ptable.lock){  //DOC: sleeplock2
2582     release(&ptable.lock);
2583     acquire(lk);
2584   }
2585 }
2586 
2587 
2588 
2589 
2590 
2591 
2592 
2593 
2594 
2595 
2596 
2597 
2598 
2599 
2600 // Wake up all processes sleeping on chan.
2601 // The ptable lock must be held.
2602 static void
2603 wakeup1(void *chan)
2604 {
2605   struct proc *p;
2606 
2607   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2608     if(p->state == SLEEPING && p->chan == chan)
2609       p->state = RUNNABLE;
2610 }
2611 
2612 // Wake up all processes sleeping on chan.
2613 void
2614 wakeup(void *chan)
2615 {
2616   acquire(&ptable.lock);
2617   wakeup1(chan);
2618   release(&ptable.lock);
2619 }
2620 
2621 // Kill the process with the given pid.
2622 // Process won't exit until it returns
2623 // to user space (see trap in trap.c).
2624 int
2625 kill(int pid)
2626 {
2627   struct proc *p;
2628 
2629   acquire(&ptable.lock);
2630   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2631     if(p->pid == pid){
2632       p->killed = 1;
2633       // Wake process from sleep if necessary.
2634       if(p->state == SLEEPING)
2635         p->state = RUNNABLE;
2636       release(&ptable.lock);
2637       return 0;
2638     }
2639   }
2640   release(&ptable.lock);
2641   return -1;
2642 }
2643 
2644 
2645 
2646 
2647 
2648 
2649 
2650 // Print a process listing to console.  For debugging.
2651 // Runs when user types ^P on console.
2652 // No lock to avoid wedging a stuck machine further.
2653 void
2654 procdump(void)
2655 {
2656   static char *states[] = {
2657   [UNUSED]    "unused",
2658   [EMBRYO]    "embryo",
2659   [SLEEPING]  "sleep ",
2660   [RUNNABLE]  "runble",
2661   [RUNNING]   "run   ",
2662   [ZOMBIE]    "zombie"
2663   };
2664   int i;
2665   struct proc *p;
2666   char *state;
2667   uint pc[10];
2668 
2669   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2670     if(p->state == UNUSED)
2671       continue;
2672     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
2673       state = states[p->state];
2674     else
2675       state = "???";
2676     cprintf("%d %s %s", p->pid, state, p->name);
2677     if(p->state == SLEEPING){
2678       getcallerpcs((uint*)p->context->ebp+2, pc);
2679       for(i=0; i<10 && pc[i] != 0; i++)
2680         cprintf(" %p", pc[i]);
2681     }
2682     cprintf("\n");
2683   }
2684 }
2685 
2686 
2687 
2688 
2689 
2690 
2691 
2692 
2693 
2694 
2695 
2696 
2697 
2698 
2699 
