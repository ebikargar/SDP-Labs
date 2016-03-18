#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

long long current_timestamp(char *s) {
	#include <sys/time.h>
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; 
    printf("%s milliseconds: %lld\n", s, milliseconds);
    printf("%s microseconds: %lld\n", s, te.tv_sec*1000000LL + te.tv_usec);
    return milliseconds;
}

void
nano_sleep(int ns){
#include <time.h>
struct timespec tim, tim_remaining;

  tim.tv_sec = 0;
  tim.tv_nsec = ns;   // 0 to  999999999 nanosecond
  nanosleep(&tim, &tim_remaining);
}

int main()
{
struct timespec tim, tim_remaining;
  tim.tv_sec = 1;
  tim.tv_nsec = 3000;
  current_timestamp("Begin: ");
  if(nanosleep(&tim , &tim_remaining) < 0 ) {
      printf("Nano sleep system call failed \n");
      return -1;
   }
   
  printf("Nano sleep successfull \n");
  current_timestamp("End:");
  
  nano_sleep(500000);
  current_timestamp("");
  printf("end nano_sleep\n");
  return 0;
}
