#include <time.h>

#include "office.h"

#define QUEUE_MAXDIM 100

typedef struct {
	pthread_mutex_t me;
	int count;
} Thread_count;
Thread_count st_count;

void *studentFunction(void *param);
void *officeFunction(void *param);
void *officeSpecialFunction(void *param);


int main(int argc, char **argv) {
	time_t t;
	int i;
	pthread_t *th_s;
	pthread_t *th_o = malloc(NUM_OFFICES * sizeof(pthread_t));
	pthread_t th_special;
	
	// check the number of parameters
	if (argc < 2) {
		printf("Provide an integer parameter k!\n");
		return -1;
	}
	k = atoi(argv[1]);
	th_s = malloc(k * sizeof(pthread_t));
	cond = cond_init(NUM_OFFICES);
	// init queues
	normal_Q = B_init(QUEUE_MAXDIM);
	special_Q = B_init(QUEUE_MAXDIM);
	urgent_Q = malloc(NUM_OFFICES * sizeof(Buffer *));
	answer_Q = malloc(k * sizeof(Buffer *));
	for(i = 0; i < k; i++) {
		answer_Q[i] = B_init(QUEUE_MAXDIM);
	}
	for(i = 0; i < NUM_OFFICES; i++) {
		urgent_Q[i] = B_init(QUEUE_MAXDIM);
	}
	
	setbuf(stdout, 0);
	srand((unsigned) time(&t));
	// initialize the counter of how may students still need to complete
	pthread_mutex_init(&st_count.me, NULL);
	st_count.count = k;
	
	int *pi;
	// create k student threads
	for(i = 0; i <k; i++) {
		pi = malloc(sizeof(int));
		*pi = i;
		pthread_create(&th_s[i], NULL, studentFunction, pi);
	}
	
	// create NUM_OFFICES office threads
	for(i = 0; i < NUM_OFFICES; i++) {
		pi = malloc(sizeof(int));
		*pi = i;
		pthread_create(&th_o[i], NULL, officeFunction, pi);
	}
	// create special office thread
	pthread_create(&th_special, NULL, officeSpecialFunction, NULL);
	
	
	// debugging purposes: print every second
	while(1) {
		printf("%d\n", (unsigned) time(&t));
		sleep(1);
	}
	// exit only this thread
	pthread_exit(0);
	// a return is needed (but will never be executed)
	return 0;
}

void *studentFunction(void *param) {
	int i = *(int *)param;
	int office_number;
	pthread_detach(pthread_self());
	sleep(rand() % 10 + 1);
	Info st_info, response;
	st_info.id = i;
	st_info.office_no = -1;
	st_info.urgent = 0;
	// critical section
	pthread_mutex_lock(&cond->lock);
	send(normal_Q, st_info);
	cond->normal++;
	pthread_cond_broadcast(cond->cond);
	pthread_mutex_unlock(&cond->lock);
	// end of critical section
	
	// received from office message
	response = receive(answer_Q[i]);
	// memorize the office number for future needs
	office_number = response.office_no;
	printf("student %d received answer from office %d\n", i, response.office_no);
	// end of service from office (can be final or special)
	response = receive(answer_Q[i]);
	if (response.office_no != -1) {
		// this is the end of the pain for this student, can go home
		printf("student %d terminated after service at office %d\n", i, response.office_no);
	} else {
		// student need to go to the special office
		printf("student %d going from normal office to the special office\n", i);
		// add the student to the special queue
		st_info.id = i;
		st_info.office_no = office_number;
		st_info.urgent = 0;
		send(special_Q, st_info);
		// wait for an accept response from the special office
		response = receive(answer_Q[i]);
		printf("student %d received by the special office\n", i);
		// wait for a response from the special office
		response = receive(answer_Q[i]);
		//printf("student %d served by the special office\n", i);
		// now need to go back to the previous office
		printf("student %d going back to office %d\n", i, office_number);
		st_info.id = i;
		st_info.office_no = office_number;
		st_info.urgent = response.urgent;
		// critical section
		pthread_mutex_lock(&cond->lock);
		send(urgent_Q[office_number], st_info);
		cond->urgent[office_number]++;
		pthread_cond_broadcast(cond->cond);
		pthread_mutex_unlock(&cond->lock);
		// end of critical section
		// wait for accept message
		response = receive(answer_Q[i]);
		printf("student %d received (urgent) from office %d\n", i, office_number);
		// and wait for the final result
		response = receive(answer_Q[i]);
		printf("student %d terminated after service at office %d\n", i, office_number);
	}
	
	// last student terminates the program (killing all of the offices)
	pthread_mutex_lock(&st_count.me);
	if (--st_count.count == 0) {
		// this is the last student
		exit(0);
	}
	pthread_mutex_unlock(&st_count.me);
	
	
	return NULL;
}
void *officeFunction(void *param) {
	int *office_no = (int *)param;
	int i = *office_no;
	Info info;
	pthread_detach(pthread_self());
	while(1) {
		while(!(cond->normal || cond->urgent[*office_no])) {
			//printf("office %d waiting on condition normal = %d, urgent[%d] = %d\n", i, cond->normal, i, cond->urgent[i]);
			pthread_cond_wait(cond->cond, &cond->lock);
		}
		if (cond->urgent[*office_no]) {
			// receive from urgent
			cond->urgent[*office_no]--;
			info = receive(urgent_Q[*office_no]);
			pthread_mutex_unlock(&cond->lock);
		} else {
			// receive from normal queue
			cond->normal--;
			info = receive(normal_Q);
			//printf("received on normal queue\n");
			pthread_mutex_unlock(&cond->lock);
		}
		Info receiveMsg, officeAnswer;
		// tell the student that this office received him
		receiveMsg.id = info.id;
		receiveMsg.office_no = i;
		receiveMsg.urgent = -1;
		send(answer_Q[info.id], receiveMsg);
		// time required to serve
		if(info.urgent == 1) {
			// this student arrives from special office, process faster
			sleep(1);
			officeAnswer.office_no = i;
		} else {
			// normal student, could send to special office
			sleep(rand() % 4 + 3);
			if (rand() % 1000 > 400) {
				// no need for additional informations
				officeAnswer.office_no = i;
			} else {
				// need to go to special office
				officeAnswer.office_no = -1;
			}
		}
		officeAnswer.id = info.id;
		officeAnswer.urgent = -1;
		// send the answer to the student
		send(answer_Q[info.id], officeAnswer);
	}
	return NULL;
}

void *officeSpecialFunction(void *param) {
	Info info, officeAnswer;
	pthread_detach(pthread_self());
	while(1) {
		info = receive(special_Q);
		officeAnswer.id = info.id;
		officeAnswer.office_no = info.office_no;
		officeAnswer.urgent = -1;
		// send a received message (the student can print that was accepted)
		send(answer_Q[info.id], officeAnswer);
		// time required to serve
		sleep(rand() % 4 + 3);
		officeAnswer.id = info.id;
		officeAnswer.office_no = info.office_no;
		officeAnswer.urgent = 1;
		// send the answer to the student
		send(answer_Q[info.id], officeAnswer);
	}
	return NULL;
}

/*
pthread_mutex_lock(&cond.lock);
while(!conditionMet) {
	printf("Thread blocked");
	pthread_cond_wait(&cond.cond, &cond.lock);
}

pthread_mutex_unlock(&cond.lock)
*/


// queue functions

/* Initialize a buffer */
Buffer *B_init (int size) {
	Buffer *b;
	b = malloc(sizeof(Buffer));
	pthread_mutex_init (&b->lock, NULL);
	b->notempty = malloc(sizeof(pthread_cond_t));
	b->notfull = malloc(sizeof(pthread_cond_t));
	pthread_cond_init (b->notempty, NULL);
	pthread_cond_init (b->notfull, NULL);
	b->out = 0;
	b->in = 0;
	b->count = 0;
	b->dim = size;
	b->buffer = malloc(size * sizeof(Info));
	return b;
}

/* Store an Info in the buffer */
void send (Buffer *b, Info data) {
	pthread_mutex_lock (&b->lock);
	/* Wait until buffer is not full */
	while (b->count == b->dim) {
		pthread_cond_wait (b->notfull, &b->lock);
		/* pthread_cond_wait reacquired b->lock before returning */
	}
	/* Write the data and advance write pointer */
	b->buffer[b->in] = data;
	b->in++;
	if (b->in >= b->dim)
	b->in = 0;
	b->count++;
	/* Signal that the buffer is now not empty */
	pthread_cond_signal (b->notempty);
	pthread_mutex_unlock (&b->lock);
}

/* Read and remove an Info from the buffer */
Info receive (Buffer *b) {
	Info data;
	pthread_mutex_lock (&b->lock);
	/* Wait until buffer is not empty */
	while (b->count == 0) {
		pthread_cond_wait (b->notempty, &b->lock);
	}
	/* Read the data and advance read pointer */
	data = b->buffer[b->out];
	b->out++;
	if (b->out >= b->dim)
	b->out = 0;
	b->count--;
	/* Signal that the buffer is now not full */
	pthread_cond_signal (b->notfull);
	pthread_mutex_unlock (&b->lock);
	return data;
}
/*
typedef struct Cond{
	pthread_mutex_t lock;  
	pthread_cond_t  *cond;	// to be allocated
	int             *urgent;	// NUM_OFFICES to be allocated
	int             normal;
} Cond;
*/
Cond * cond_init(int n) {
	Cond *c;
	int i;
	c = malloc(sizeof(Cond));
	c->cond = malloc(sizeof(pthread_cond_t));
	pthread_mutex_init (&c->lock, NULL);
	pthread_cond_init (c->cond, NULL);
	c->urgent = malloc(n * sizeof(int));
	c->normal = 0;
	for(i = 0; i < n; i++) {
		c->urgent[i] = 0;
	}
	return c;
}
