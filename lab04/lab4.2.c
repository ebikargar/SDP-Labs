#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <semaphore.h>

typedef struct {
  int *vet;
  int left;
  int right;
} mergeParamType;

int swapsCount;

int runningThreadsSorting;
pthread_mutex_t runningThreadsSortingME;

// the last thread needs to know all of the boundaries...
mergeParamType *params;
int nthreads;

sem_t sem1, sem2;

void inefficientSort(int *v, int dim);
void bubbleSort(mergeParamType *param);
void *threadFunction(void * param);

int main (int argc, char ** argv)
{
    int n, len;
    struct stat stat_buf;
    int fd;
    char *paddr;
    int *v;

    if (argc != 2) {
        printf ("Syntax: %s filename\n", argv[0]);
        return (1);
    }

    if ((fd = open (argv[1], O_RDWR)) == -1)
        perror ("open");

    if (fstat (fd, &stat_buf) == -1)
        perror ("fstat");
    // size of the file
    len = stat_buf.st_size;
    // how many int in the file?
    n = len / sizeof (int);
    // MAP_SHARED is needed to make the updates in the mapped array visible to other processes and to propagate modifications to the disk
    paddr = mmap ((caddr_t) 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (paddr == (caddr_t) - 1)
        perror ("mmap");

    close (fd);
    v = (int *) paddr;
    
    inefficientSort(v, n);
    //only terminate this thread
    pthread_exit(0);
    printf("i am not executed!\n");
    // this return is only needed from the signature of the main method, is never executed
    // because of pthread_exit
    return 0;
}

void inefficientSort(int *v, int dim) {

    int i;
    pthread_t *threads;
    // formula to calculate number of threads to be executed
    nthreads = ceil(log10(dim));
    // how many elements each thread has to manage
    int region_size = dim / nthreads;
    
    threads = malloc(nthreads * sizeof(pthread_t));
    params = malloc (nthreads * sizeof(mergeParamType));
    
    setbuf(stdout, 0);
    
    pthread_mutex_init(&runningThreadsSortingME, NULL);
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 0);
    runningThreadsSorting = nthreads;
    setbuf(stdout, 0);
    // prepare and launch the execution of threads
    for (i = 0; i < nthreads; i++) {
        params[i].vet = v;
        params[i].left = region_size * i;
        if(i == nthreads - 1) {
            // the rightmost thread goes till the end of the array
            params[i].right = dim - 1;
        } else {
            params[i].right = region_size * (i + 1) -1;
        }
        //printf("Created params: left %d right %d\n", params[i].left, params[i].right);
        pthread_create(&threads[i], NULL, threadFunction, &params[i]);
    }
}

// this function performs a simple bubble sort on the specified region of the array
void bubbleSort(mergeParamType *p) {
    int *vet = p->vet;
    int l = p->left;
    int r = p->right;
    int i, j, temp;
    for (i = l; i < r; i++) {
      for (j = l; j < r - i + l; j++) {
        if (vet[j] > vet[j+1]) {
          temp = vet[j];
          vet[j] = vet[j+1];
          vet[j+1] = temp;
        }
      } 
    }
    return;
}

// this function will be executed by each thread
void *threadFunction(void * param) {
    
    mergeParamType *p = (mergeParamType *)param;
    
    /*
     * main loop:
     * while some swaps occurred
     * run the local sort
     * the last thread must do:
     * check, swap and update the swapsCount global variable for all the boundaries
    */
    int i, j = 0, k;
    
    pthread_detach(pthread_self());
    
    do {
        // sleep randomly to test barrier mechanism
        //sleep(rand() % 5);
        //printf("iteration: %d left:%d\n", j, p->left);
        
        // perform sort on the assigned region
        bubbleSort(param);
        
        // this is the first barrier
        pthread_mutex_lock(&runningThreadsSortingME);
        k = --runningThreadsSorting;
        //printf("k = %d by thread %d\n", k, p->left);
        pthread_mutex_unlock(&runningThreadsSortingME);
        if (k == 0) {
            // this is the last thread
            //printf("last thread is checking boundaries..\n");
            swapsCount = 0;
            // for each boundary
            for(i = 0; i < nthreads - 1; i++) {
                // extreme will contain the index of the left part of a boundary
                int extreme = params[i].right;
                //printf("extreme = %d, values: %d %d\n", extreme, p->vet[extreme], p->vet[extreme + 1]);
                // check if a swap is needed
                if (p->vet[extreme] > p->vet[extreme + 1]) {
                    // perform a swap
                    int temp = p->vet[extreme];
                    p->vet[extreme] = p->vet[extreme + 1];
                    p->vet[extreme + 1] = temp;
                    // update the number of swaps
                    swapsCount++;
                }
            }
            // now it's time to tell every thread that this step is finished
            for(i = 0; i < nthreads; i++) {
                //printf("post\n");
                sem_post(&sem1);
            }
        }
        //printf("waiting..\n");
        sem_wait(&sem1);
                
        // this second barrier is needed because the threads are cyclic
        pthread_mutex_lock(&runningThreadsSortingME);
        k = ++runningThreadsSorting;
        //printf("k = %d by thread %d\n", k, p->left);
        pthread_mutex_unlock(&runningThreadsSortingME);
        if(k == nthreads) {
            for(i = 0; i < nthreads; i++) {
                sem_post(&sem2);
            }
        }
        sem_wait(&sem2);
        
        j++;
    }
    while(swapsCount);
    
    return NULL;
}
