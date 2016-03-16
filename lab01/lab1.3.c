#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

void *client_thread(void *arg);
void server_task();

// global variable to be protected
int val;
// global semaphores for val
// sem_write is to execute complete transactions in mutual exclusion
// (from first write done by client, successive update from server and final read from client)
//
// sem_read is used to inform the server that can read a value and process it
//
// sem_ready is used to inform the client that the value is ready (processed by server)
// only a client is in the critical section using the global variable, so the value will be received
// from the right client
sem_t sem_write, sem_read, sem_ready;
// protect the finished variable
sem_t finishedME;
int finished = 0;

int main(int argc, char ** argv) {
	int retcode_a, retcode_b;
	
	pthread_t th_a, th_b;
	// initially a client can start a transaction, using shared variable
	sem_init(&sem_write, 0, 1);
	// initially server must wait to read shared variable
	sem_init(&sem_read, 0, 0);
	// initially the variable is not yet processed by server
	sem_init(&sem_ready, 0, 0);
	// this is a mutex semaphore
	sem_init(&finishedME, 0, 1);
	retcode_a = pthread_create(&th_a, NULL, client_thread, (void *) "fv1.b");
	retcode_b = pthread_create(&th_b, NULL, client_thread, (void *) "fv2.b");
	if(retcode_a != 0 || retcode_b != 0) {
		printf("Error creating one or more threads\n");
		exit(1);
	}
	// the main thread will act as server
	server_task();
	pthread_join(th_a, NULL);
	pthread_join(th_b, NULL);
	return 0;
}

void *client_thread(void* arg) {
	char * filename = (char *) arg;
	int number;
	int fd = open(filename, O_RDONLY);
	while(read(fd, &number, sizeof(int))) {
		// to be sure that can write
		sem_wait(&sem_write);
		val = number;
		// now server can read
		sem_post(&sem_read);
		// and client needs to wait end of processing
		sem_wait(&sem_ready);
		// now can print out
		printf("result: %d id: %u\n", val, (int) pthread_self());
		fflush(stdout);
		
		// another client can start a transaction with server
		sem_post(&sem_write);
		// to check that clients can run interleaved
		sleep(1);
	}
	sem_wait(&finishedME);
	finished++;
	if (finished == 2) {
		//the last server needs to wake up server
		sem_post(&sem_read);
	}
	sem_post(&finishedME);
	return arg;
}

void server_task() {
	//int finished_copy;
	//sem_wait(&finishedME);
	//finished_copy = finished;
	//sem_post(&finishedME);
	int n_req = 0;
	
	while(1) {
		sem_wait(&sem_read);
		sem_wait(&finishedME);
		if (finished < 2) {
			val <<= 1;
			sem_post(&sem_ready);
			n_req++;
		} else {
			printf("server ended, n_req=%d\n", n_req);
			break;
		}
		sem_post(&finishedME);
	}
	//exit(0);
}
