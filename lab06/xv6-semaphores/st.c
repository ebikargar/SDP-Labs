#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "spinlock.h"

char buf[8192];
char name[3];
int stdout = 1;

void
semaphore_test(void) {
  int pid;
  int sem;

  sem = sem_alloc();
  sem_init(sem, 0);
  
  printf(stdout, "A\n");
  pid = fork();
  if (pid) {
//	  sleep(5);
   sem_wait(sem);
   printf(stdout, "C\n");
   wait();
   return;
   sem_destroy(sem);
  }
  else {
    sleep(5);
    printf (stdout,  "B\n");
    sem_post(sem);
    printf (stdout, "D\n");
    exit();
  }
}


int
main(int argc, char *argv[])
{
  semaphore_test();
  exit();
}
