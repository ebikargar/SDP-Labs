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

void condition_test1(void) {
  int pid = 0;
  int cond, value;

  cond = cond_alloc();
  cond_set(cond, 0);
	//printf(stdout, "Condition %d has value: %d\n", cond, cond_get(cond));
  pid = fork();
  if (pid) {
		// the parent
		value = 0;
		while(1) {
			// why sleep is not working?
			sleep(5);
			value++;
			cond_set(cond, value);
			printf(stdout, "Condition %d set on value: %d\n", cond, cond_get(cond));
			cond_signal(cond);
			if(value == 10) {
				break;
			}
		}
		printf(stdout, "parent end\n");
		wait();
  }
  else {
		// the child
		// should acquire a lock (need to define a function cond_lock that locks internally m)
		cond_lock(cond);
		while((value = cond_get(cond)) != 10) {
			printf(stdout, "Waiting on cond %d because of value %d\n", cond, value);
			// cond_wait also unlocks internal lock m, that is reaquired on wakeup
			cond_wait(cond);
		}
		cond_unlock(cond);
		printf(stdout, "Condition passed! value = %d\n", value);
		exit();
  }
	printf(stdout, "cond_test1 passed!\n");
	cond_destroy(cond);
}

void condition_test2(void) {
  int pid, cpid;
  int cond, value;

  cond = cond_alloc();
  cond_set(cond, 0);
	//printf(stdout, "Condition %d has value: %d\n", cond, cond_get(cond));
  cpid = fork();
  if (cpid) {
		// the parent
		value = 0;
		while(1) {
			// why sleep is not working?
			sleep(5);
			value++;
			cond_set(cond, value);
			printf(stdout, "Condition %d set on value: %d\n", cond, cond_get(cond));
			//cond_signal(cond);
			if(value == 10) {
        cond_broadcast(cond);
				break;
			} else {
        cond_signal(cond);
      }
		}
		printf(stdout, "parent end\n");
		wait();
  }
  else {
		// the child
		// should acquire a lock (need to define a function cond_lock that locks internally m)
    cpid = fork();
    pid = getpid();
		cond_lock(cond);
		while((value = cond_get(cond)) != 10) {
			printf(stdout, "%d Waiting on cond %d because of value %d\n", pid, cond, value);
			// cond_wait also unlocks internal lock m, that is reaquired on wakeup
			cond_wait(cond);
		}
		cond_unlock(cond);
		printf(stdout, "%d Condition passed! value = %d\n", pid, value);
    if(cpid) {
      wait();
    }
    exit();
  }
	printf(stdout, "cond_test2 passed!\n");
	cond_destroy(cond);
}

void mutex_test1(void) {
  int cond, pid;
  cond = cond_alloc();
  cond_set(cond, 0);
  cond_lock(cond);
  pid = fork();
  if(pid) {
    // parent
    printf(stdout, "1 (parent)\n");
    cond_unlock(cond);
    wait();
  } else {
    // child
    cond_lock(cond);
    printf(stdout, "2 (child)\n");
    cond_unlock(cond);
    exit();
  }
  cond_destroy(cond);
  printf(stdout, "mutex_test1 passed!\n");
}

void mutex_test2(void) {
  int cond1, cond2, pid;
  cond1 = cond_alloc();
  cond2 = cond_alloc();
  cond_set(cond1, 0);
  cond_set(cond2, 0);
  cond_lock(cond1);
  cond_lock(cond2);
  pid = fork();
  if(pid) {
    // parent
    printf(stdout, "1 (parent)\n");
    cond_unlock(cond1);
    wait();
  } else {
    // child
    if(fork()) {
      cond_lock(cond1);
      printf(stdout, "2 (child 1)\n");
      cond_unlock(cond1);
      cond_lock(cond2);
      printf(stdout, "4 (child 1)\n");
      cond_unlock(cond2);
      wait();
    } else {
      cond_lock(cond1);
      printf(stdout,"3 (child 2)\n");
      cond_unlock(cond1);
      cond_unlock(cond2);
      exit();
    }
    exit();
  }
  cond_destroy(cond1);
  cond_destroy(cond2);
  printf(stdout, "mutex_test2 passed!\n");
}

void condition_test3(void) {
  int cond, val, sem;
  cond = cond_alloc();
  cond_set(cond, -1);
  sem = sem_alloc();
  sem_init(sem, 0);
  if(fork()) {
    // parent
    cond_lock(cond);
    while((val = cond_get(cond)) < 0) {
      printf(stdout, "parent going to wait, value = %d\n", val);
      sem_post(sem);
      cond_wait(cond);
      printf(stdout, "parent woken up\n");
    }
    cond_unlock(cond);
    sem_post(sem);
    printf(stdout, "parent passed the while loop, value = %d\n", val);
    
    wait();
  } else {
    // child 1
    if(fork()) {
      // child 1
      cond_lock(cond);
      while((val = cond_get(cond)) < 1) {
        printf(stdout, "child 1 going to wait, value = %d\n", val);
        sem_post(sem);
        cond_wait(cond);
        printf(stdout, "child 1 woken up\n");
      }
      cond_unlock(cond);
      sem_post(sem);
      printf(stdout, "child 1 passed the while loop, value = %d\n", val);
      
      wait();
      exit();
      printf(stdout, "child 1 terminating\n");
    } else {
      // child 2
      if(fork()) {
          // child 2
          cond_lock(cond);
          while((val = cond_get(cond)) < 2) {
            printf(stdout, "child 2 going to wait, value = %d\n", val);
            sem_post(sem);
            cond_wait(cond);
            printf(stdout, "child 2 woken up\n");
          }
          cond_unlock(cond);
          sem_post(sem); 
          printf(stdout, "child 2 passed the while loop, value = %d\n", val);
          
          wait();
          exit();
          printf(stdout, "child 2 terminating\n");  
      } else {
          //child 3
          sem_wait(sem);
          sem_wait(sem);
          sem_wait(sem);  // wait the other 3 
          cond_signal(cond);
          printf(stdout, "child 3 sent a signal\n");
          sem_wait(sem);
          cond_set(cond, -2);
          printf(stdout, "child 3 set value to %d\n", cond_get(cond));
          cond_signal(cond);
          cond_signal(cond);
          printf(stdout, "child 3 sent two signals\n");
          sem_wait(sem);
          sem_wait(sem);
          cond_set(cond, 0);
          printf(stdout, "child 3 set value to %d\n", cond_get(cond));
          cond_broadcast(cond);
          printf(stdout, "child 3 sent broadcast\n");
          sem_wait(sem);
          sem_wait(sem);
          sem_wait(sem);
          cond_set(cond, 1);
          printf(stdout, "child 3 set value to %d\n", cond_get(cond));
          cond_broadcast(cond);
          printf(stdout, "child 3 sent broadcast\n");
          sem_wait(sem);
          sem_wait(sem);
          cond_set(cond, 2);
          printf(stdout, "child 3 set value to %d\n", cond_get(cond));
          cond_signal(cond);
          printf(stdout, "child 3 sent signal\n");
          sem_wait(sem);
          printf(stdout, "child 3 terminating\n");
          exit();
      }
    }
  }
  cond_destroy(cond);
  sem_destroy(sem);
  printf(stdout, "cond_test3 passed!\n");
}

int
main(int argc, char *argv[])
{
	condition_test1();
  mutex_test1();
  mutex_test2();
  condition_test2();
  condition_test3();
  exit();
}
