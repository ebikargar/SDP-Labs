#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 512
#define SECONDS 60

void sigusr1Handler(int);
void sigusr2Handler(int);

// waiting variable will be used only by parent
int waiting = 0;
int cpid = 0;

int main(int argc, char **argv) {
    // check on number of parameters
    if(argc < 2) {
        printf("Please provide as parameter the file to be read\n");
        return -1;
    }
    
    cpid = fork();
    if(cpid) {
        // parent
        FILE *fp = fopen(argv[1], "rt");
        if(fp == NULL) {
            printf("Error opening file\n");
            // kill also the child
            kill(cpid, SIGKILL);
            return -1;
        }
        char *buffer = malloc(BUFFER_SIZE * sizeof(char));
        int i;
        signal(SIGUSR1, sigusr1Handler);
        signal(SIGUSR2, sigusr2Handler);
        while(1) {
            // this loop repeats the whole file read
            i = 0;
            while(fgets(buffer, BUFFER_SIZE, fp)) {
                // this inner loop iterates on the lines of the file
                if(waiting) {
                    pause();
                }
                printf("%d:  %s", ++i, buffer);
                fflush(stdout);
            }
            rewind(fp);
        }
    } else {
        // child
        int secs_remaining = SECONDS;
        int rand_secs;
        time_t t;
        srand((unsigned) time(&t));
        while(secs_remaining > 0) {
            if(secs_remaining < 10) {
                rand_secs = secs_remaining;
            } else {
                rand_secs = rand() % 10 + 1;
            }
            secs_remaining -= rand_secs;
            sleep(rand_secs);
            // send SIGUSR1 to parent
            kill(getppid(), SIGUSR1);
            //printf("\nStill remaining %d seconds\n", secs_remaining);
        }
        //printf("%d: I'm killing my parent %d\n", getpid(), getppid());
        kill(getppid(), SIGUSR2);
    }
    return 0;
}

void sigusr1Handler(int signo) {
    waiting = ++waiting % 2;
    signal(SIGUSR1, sigusr1Handler);
    //printf("%d: I received SIGUSR1 from my child %d\n", getpid(), cpid);
}

void sigusr2Handler(int signo) {
    //printf("%d: I received SIGUSR2 from my child %d\n", getpid(), cpid);
    exit(0);
}
