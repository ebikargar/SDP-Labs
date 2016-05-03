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
semaphore_test1(void) {
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
  sem_destroy(sem);
}

void semaphore_test2() {
	int pid, pid2;
	int sem;

	sem = sem_alloc();
	sem_init(sem, 0);

	pid = fork();
	if(pid) {
		// father
		sem_wait(sem);
		printf(stdout, "parent woken\n");
    sem_post(sem);
		wait();
	} else {
		// child
		pid2 = fork();
		if(pid2) {
			sem_wait(sem);
			printf(stdout, "child 1 woken\n");
      sem_post(sem);
			wait();
      exit();
		} else {
			sem_post(sem);
			printf(stdout, "posted\n");
      exit();
		}
	}
  printf(stdout, "passed sem_test2!\n");
  sem_destroy(sem);
}

void semaphore_test3(void) {
  int sem1, sem2;
  int i;
  sem1 = sem_alloc();
  sem2 = sem_alloc();
  sem_init(sem1, 0);
  sem_init(sem2, 0);
  sem_post(sem2);
  
  if(fork()) {
    // parent
    for(i = 0; i < 10; i++) {
      sem_wait(sem1); // wait for a child
      printf(stdout, "parent %d\n", i);
      sem_post(sem2); // wake a child
    }
    wait();
  } else {
    // child1
    if(fork()) {
      // child1
      for(i = 0; i < 5; i++) {
        sem_wait(sem2);
        printf(stdout, "child1 %d\n", i);
        sem_post(sem1);
      }
      wait();
      exit();
    } else {
      // child2
      for(i = 0; i < 5; i++) {
        sem_wait(sem2);
        printf(stdout, "child2 %d\n", i);
        sem_post(sem1);
      }
      exit();
    }
  }
  printf(stdout, "sem_test3 passed!\n");
}

int
main(int argc, char *argv[])
{
  semaphore_test1();
  semaphore_test2();
  semaphore_test3();
  exit();
}
