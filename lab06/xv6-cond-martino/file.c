//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "file.h"
#include "spinlock.h"

struct semaphore {
  struct spinlock lock;
  int count; // can also be negative
  int tokens; // tokens for resuming execution
  int ref; //assigned or not
  uint q;
};

struct {
  struct spinlock lock;
  struct semaphore semaphore[NSEM_MAX];
} semaphore_table;

//***************************************************
struct condition {
  struct spinlock lock;
  int locked; // refers to the internal lock (accessible via cond_lock and cond_unlock)
  int count; // how many processes waiting on condition variable
  int resurrection_token; // these tokens are taken by processes waking up, so if there is only a token, only a process can wake up
  int value; // the internal value (accessible via cond_get and cond_set)
  int ref; //assigned or not (used for allocating/freeing a condition)
  uint q; //channel for waiting
  uint m; //another channel, for the lock waiting
};
struct {
  struct spinlock lock;
  struct condition condition[NCOND_MAX];
} condition_table;
//**************************************************

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}
void
semaphore_init(void)
{
  initlock(&semaphore_table.lock, "semaphore_table");
}

//********************************************************
void condition_init(void)
{
  initlock(&condition_table.lock, "condition_table");
}
//********************************************************

// Allocate a semaphore structure.
int
sem_alloc(void)
{
  struct semaphore *s;
  int k = 0;

  acquire(&semaphore_table.lock);
  for(s = semaphore_table.semaphore; s < semaphore_table.semaphore + NSEM_MAX; s++){
    if(s->ref == 0){
      s->ref = 1;
      release(&semaphore_table.lock);
      return k;
    }
		k++;
  }
  release(&semaphore_table.lock);
  return -1;
}

int
sem_init(int sn, int n)
{
  struct semaphore *s;
  
  s = &semaphore_table.semaphore[sn];
  acquire(&s->lock);
  s->count = s->tokens = n;
  release(&s->lock);
  return 0;
}

int
sem_destroy(int sn)
{
	 struct semaphore *s;
  
  s = &semaphore_table.semaphore[sn];
  acquire(&semaphore_table.lock); // s->ref should be accessed in mutual exclusion (also sem_alloc uses it)
  s->ref = 0;
  release(&semaphore_table.lock);
  return 0;
}

int
sem_wait(int sn)
{  
  struct semaphore *s;
  
  s = &semaphore_table.semaphore[sn];
  acquire(&s->lock);
  s->count--; // the count is updated immediately
  while(s->tokens < 1) {
    sleep(&s->q, &s->lock);
  }
  s->tokens--;  // consume a token (when resuming execution)

  // BUG found in previous implementation: if on &s->q more than one is sleeping, everyone of them
  // continues execution after a post
  /*if (s->count < 0)
    sleep(&s->q, &s->lock);
  */
  release(&s->lock);
  return 0;
}

int
sem_post(int sn)
{
  struct semaphore *s;
    
  s = &semaphore_table.semaphore[sn];
      
  acquire(&s->lock);
  s->count++;
  s->tokens++;
  if (s->count <= 0)  // there is someone waiting
    wakeup(&s->q);
  release(&s->lock); 
  return 0;
}

//**************************************************************************
int cond_alloc(void) {
  struct condition *c;
  int k = 0;

  acquire(&condition_table.lock);
  for(c = condition_table.condition; c < condition_table.condition + NCOND_MAX; c++) {
    if(c->ref == 0){
      c->ref = 1; // this condition is now used
      c->count = 0; // initialize the count to 0
      c->resurrection_token = 0; // no one is allowed to resurrect
      initlock(&c->lock, "lock");
      c->locked = 0; // the initial status of the lock: someone can acquire it
      release(&condition_table.lock);
      return k;
    }
		k++;
  }
  release(&condition_table.lock);
  return -1;
}

void cond_set(int cn, int n) {
  struct condition *c;
  
  c = &condition_table.condition[cn];
  acquire(&c->lock);
  c->value = n; // set the value
  release(&c->lock);
}
int cond_get(int cn) {
  struct condition *c;
  int n;
  c = &condition_table.condition[cn];
  acquire(&c->lock);
  n = c->value; // get the value
  release(&c->lock);
  return n;
}

void cond_destroy(int cn) {
  struct condition *c;
  c = &condition_table.condition[cn];
  acquire(&condition_table.lock); // need to modify c->ref, better to do in mutual exclusion with alloc
  c->ref = 0; // this condition is no more used
  release(&condition_table.lock);
}

void cond_lock(int cn) {
  struct condition *c;
  c = &condition_table.condition[cn];
  acquire(&c->lock);

  //need WHILE instead of IF because wakeup wakes up everyone on &c->m
  while(c->locked) {
    sleep(&c->m, &c->lock); // go to sleep until someone wakes up the processes on &c->m
  }
  // set c->locked to 1 so no other process can exit the preceding while loop
  c->locked = 1;
  release(&c->lock);
}

void cond_unlock(int cn) {
  struct condition *c;
  c = &condition_table.condition[cn];
  acquire(&c->lock);

  if(c->locked) {
    c->locked = 0; // by setting this, another process can acquire the lock
    wakeup(&c->m); // wakeup everyone on &c->m, but only one of them will be able to acquire the lock
  }
  release(&c->lock);
}

void cond_wait(int cn)
{  
  struct condition *c;
  
  c = &condition_table.condition[cn];
  acquire(&c->lock);
  c->count++; // this process must be counted
  // unlock the lock (same code as above in cond_unlock)
  if(c->locked) {
    c->locked = 0;
    wakeup(&c->m);
  }
  // the WHILE puts back to sleep all the processes that have been woken up but weren't able to get a resurrection token
  while(c->resurrection_token == 0) {
    sleep(&c->q, &c->lock);
  }
  // take a resurrection token, those are so limited!
  c->resurrection_token--;
  c->count--; // update the count of waiting processes
  
  // same code as in cond_lock
  // WHILE instead of IF, makes sleep again processes that weren't able to acquire the lock
  while(c->locked) {
    sleep(&c->m, &c->lock);
  }
  // set c->locked to 1 so no other process can exit the preceding while loop
  c->locked = 1;

  release(&c->lock);
}

void cond_signal(int cn)
{
  struct condition *c;
    
  c = &condition_table.condition[cn];
  acquire(&c->lock);
  // if there is someone waiting
  if (c->count > 0) {
    // if the number of processes waiting is greater than the number of resurrection tokens
    if(c->count > c->resurrection_token) {
      // i give away a token for a lucky process
      c->resurrection_token++;
    }
    wakeup(&c->q);  // and wake up everyone on &c->q, but only one of them will get the token
  }
  release(&c->lock); 
}

void cond_broadcast(int cn)
{
  struct condition *c;
    
  c = &condition_table.condition[cn];
  acquire(&c->lock);
  // if there is someone waiting
  if (c->count > 0) {
    // this time i am generous: a resurrection token for every process waiting!!
    c->resurrection_token = c->count;
    wakeup(&c->q); // wake up everyone: each one of them will consume a resurrection token
  }
  release(&c->lock); 
}
//*************************************************************************

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);
  
  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_trans();
    iput(ff.ip);
    commit_trans();
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((LOGSIZE-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_trans();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      commit_trans();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}

