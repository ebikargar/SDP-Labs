7200 // Console input and output.
7201 // Input is from the keyboard or serial port.
7202 // Output is written to the screen and serial port.
7203 
7204 #include "types.h"
7205 #include "defs.h"
7206 #include "param.h"
7207 #include "traps.h"
7208 #include "spinlock.h"
7209 #include "fs.h"
7210 #include "file.h"
7211 #include "memlayout.h"
7212 #include "mmu.h"
7213 #include "proc.h"
7214 #include "x86.h"
7215 
7216 static void consputc(int);
7217 
7218 static int panicked = 0;
7219 
7220 static struct {
7221   struct spinlock lock;
7222   int locking;
7223 } cons;
7224 
7225 static void
7226 printint(int xx, int base, int sign)
7227 {
7228   static char digits[] = "0123456789abcdef";
7229   char buf[16];
7230   int i;
7231   uint x;
7232 
7233   if(sign && (sign = xx < 0))
7234     x = -xx;
7235   else
7236     x = xx;
7237 
7238   i = 0;
7239   do{
7240     buf[i++] = digits[x % base];
7241   }while((x /= base) != 0);
7242 
7243   if(sign)
7244     buf[i++] = '-';
7245 
7246   while(--i >= 0)
7247     consputc(buf[i]);
7248 }
7249 
7250 // Print to the console. only understands %d, %x, %p, %s.
7251 void
7252 cprintf(char *fmt, ...)
7253 {
7254   int i, c, locking;
7255   uint *argp;
7256   char *s;
7257 
7258   locking = cons.locking;
7259   if(locking)
7260     acquire(&cons.lock);
7261 
7262   if (fmt == 0)
7263     panic("null fmt");
7264 
7265   argp = (uint*)(void*)(&fmt + 1);
7266   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
7267     if(c != '%'){
7268       consputc(c);
7269       continue;
7270     }
7271     c = fmt[++i] & 0xff;
7272     if(c == 0)
7273       break;
7274     switch(c){
7275     case 'd':
7276       printint(*argp++, 10, 1);
7277       break;
7278     case 'x':
7279     case 'p':
7280       printint(*argp++, 16, 0);
7281       break;
7282     case 's':
7283       if((s = (char*)*argp++) == 0)
7284         s = "(null)";
7285       for(; *s; s++)
7286         consputc(*s);
7287       break;
7288     case '%':
7289       consputc('%');
7290       break;
7291     default:
7292       // Print unknown % sequence to draw attention.
7293       consputc('%');
7294       consputc(c);
7295       break;
7296     }
7297   }
7298 
7299 
7300   if(locking)
7301     release(&cons.lock);
7302 }
7303 
7304 void
7305 panic(char *s)
7306 {
7307   int i;
7308   uint pcs[10];
7309 
7310   cli();
7311   cons.locking = 0;
7312   cprintf("cpu%d: panic: ", cpu->id);
7313   cprintf(s);
7314   cprintf("\n");
7315   getcallerpcs(&s, pcs);
7316   for(i=0; i<10; i++)
7317     cprintf(" %p", pcs[i]);
7318   panicked = 1; // freeze other CPU
7319   for(;;)
7320     ;
7321 }
7322 
7323 
7324 
7325 
7326 
7327 
7328 
7329 
7330 
7331 
7332 
7333 
7334 
7335 
7336 
7337 
7338 
7339 
7340 
7341 
7342 
7343 
7344 
7345 
7346 
7347 
7348 
7349 
7350 #define BACKSPACE 0x100
7351 #define CRTPORT 0x3d4
7352 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
7353 
7354 static void
7355 cgaputc(int c)
7356 {
7357   int pos;
7358 
7359   // Cursor position: col + 80*row.
7360   outb(CRTPORT, 14);
7361   pos = inb(CRTPORT+1) << 8;
7362   outb(CRTPORT, 15);
7363   pos |= inb(CRTPORT+1);
7364 
7365   if(c == '\n')
7366     pos += 80 - pos%80;
7367   else if(c == BACKSPACE){
7368     if(pos > 0) --pos;
7369   } else
7370     crt[pos++] = (c&0xff) | 0x0700;  // black on white
7371 
7372   if((pos/80) >= 24){  // Scroll up.
7373     memmove(crt, crt+80, sizeof(crt[0])*23*80);
7374     pos -= 80;
7375     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
7376   }
7377 
7378   outb(CRTPORT, 14);
7379   outb(CRTPORT+1, pos>>8);
7380   outb(CRTPORT, 15);
7381   outb(CRTPORT+1, pos);
7382   crt[pos] = ' ' | 0x0700;
7383 }
7384 
7385 void
7386 consputc(int c)
7387 {
7388   if(panicked){
7389     cli();
7390     for(;;)
7391       ;
7392   }
7393 
7394   if(c == BACKSPACE){
7395     uartputc('\b'); uartputc(' '); uartputc('\b');
7396   } else
7397     uartputc(c);
7398   cgaputc(c);
7399 }
7400 #define INPUT_BUF 128
7401 struct {
7402   struct spinlock lock;
7403   char buf[INPUT_BUF];
7404   uint r;  // Read index
7405   uint w;  // Write index
7406   uint e;  // Edit index
7407 } input;
7408 
7409 #define C(x)  ((x)-'@')  // Control-x
7410 
7411 void
7412 consoleintr(int (*getc)(void))
7413 {
7414   int c;
7415 
7416   acquire(&input.lock);
7417   while((c = getc()) >= 0){
7418     switch(c){
7419     case C('P'):  // Process listing.
7420       procdump();
7421       break;
7422     case C('U'):  // Kill line.
7423       while(input.e != input.w &&
7424             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
7425         input.e--;
7426         consputc(BACKSPACE);
7427       }
7428       break;
7429     case C('H'): case '\x7f':  // Backspace
7430       if(input.e != input.w){
7431         input.e--;
7432         consputc(BACKSPACE);
7433       }
7434       break;
7435     default:
7436       if(c != 0 && input.e-input.r < INPUT_BUF){
7437         c = (c == '\r') ? '\n' : c;
7438         input.buf[input.e++ % INPUT_BUF] = c;
7439         consputc(c);
7440         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
7441           input.w = input.e;
7442           wakeup(&input.r);
7443         }
7444       }
7445       break;
7446     }
7447   }
7448   release(&input.lock);
7449 }
7450 int
7451 consoleread(struct inode *ip, char *dst, int n)
7452 {
7453   uint target;
7454   int c;
7455 
7456   iunlock(ip);
7457   target = n;
7458   acquire(&input.lock);
7459   while(n > 0){
7460     while(input.r == input.w){
7461       if(proc->killed){
7462         release(&input.lock);
7463         ilock(ip);
7464         return -1;
7465       }
7466       sleep(&input.r, &input.lock);
7467     }
7468     c = input.buf[input.r++ % INPUT_BUF];
7469     if(c == C('D')){  // EOF
7470       if(n < target){
7471         // Save ^D for next time, to make sure
7472         // caller gets a 0-byte result.
7473         input.r--;
7474       }
7475       break;
7476     }
7477     *dst++ = c;
7478     --n;
7479     if(c == '\n')
7480       break;
7481   }
7482   release(&input.lock);
7483   ilock(ip);
7484 
7485   return target - n;
7486 }
7487 
7488 
7489 
7490 
7491 
7492 
7493 
7494 
7495 
7496 
7497 
7498 
7499 
7500 int
7501 consolewrite(struct inode *ip, char *buf, int n)
7502 {
7503   int i;
7504 
7505   iunlock(ip);
7506   acquire(&cons.lock);
7507   for(i = 0; i < n; i++)
7508     consputc(buf[i] & 0xff);
7509   release(&cons.lock);
7510   ilock(ip);
7511 
7512   return n;
7513 }
7514 
7515 void
7516 consoleinit(void)
7517 {
7518   initlock(&cons.lock, "console");
7519   initlock(&input.lock, "input");
7520 
7521   devsw[CONSOLE].write = consolewrite;
7522   devsw[CONSOLE].read = consoleread;
7523   cons.locking = 1;
7524 
7525   picenable(IRQ_KBD);
7526   ioapicenable(IRQ_KBD, 0);
7527 }
7528 
7529 
7530 
7531 
7532 
7533 
7534 
7535 
7536 
7537 
7538 
7539 
7540 
7541 
7542 
7543 
7544 
7545 
7546 
7547 
7548 
7549 
