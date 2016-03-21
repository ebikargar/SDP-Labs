#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <pthread.h>
#include <semaphore.h>

/* doubleVettToVerticalMatrix transforms an array of size dim into a 
*  vertical matrix (dim x 1)
*/
double **doubleVettToVerticalMatrix(double *vett, int dim);

/* doubleMatrixTranspone does the transposition of a matrix.
*  mat (m * n) --> result (n * m)
*/
double **doubleMatrixTranspone(double **mat, int m, int n);

/* doubleMatrixPrint receives a matrix as a parameter of size (m x n)
*  and prints the values
*/
void doubleMatrixPrint(double **mat, int m, int n);


double *v1, *v2, *v;
double **mat, **m1, **m1t, **m2;
double result;
int k;

int n_thread_left;
// mutex for accessing variable n_thread_left
pthread_mutex_t n_thread_mutex;

void *thread_function(void *param);

int main(int argc, char **argv) {

	int retval;
	int i, j;

	if (argc < 2) {
		printf("Please provide an integer parameter k\n");
		return -1;
	}

	k = atoi(argv[1]);
	if (k == 0) {
		printf("Provide a non-zero parameter!\n");
		return -1;
	}

	srand(k);

	v1 = malloc(k * sizeof(double));
	v2 = malloc(k * sizeof(double));
	mat = malloc(k * sizeof(double *));

	// assign values to arrays and to the matrix
	for (i = 0; i < k; i++) {
		v1[i] = ((double)rand()) / INT_MAX - 0.5;
		v2[i] = ((double)rand()) / INT_MAX - 0.5;
		mat[i] = malloc(k * sizeof(double));
		for (j = 0; j < k; j++) {
			mat[i][j] = ((double)rand()) / INT_MAX - 0.5;
		}
	}

	v = malloc(k * sizeof(double));
	
	// transform v1 into a matrix of size (k x 1)
	m1 = doubleVettToVerticalMatrix(v1, k);
	// transpone m1
	m1t = doubleMatrixTranspone(m1, k, 1);
	printf("Matrix m1t:\n");
	doubleMatrixPrint(m1t, 1, k);
	printf("Matrix mat:\n");
	doubleMatrixPrint(mat, k, k);
	// transform v2 into a matrix of size (k x 1)
	m2 = doubleVettToVerticalMatrix(v2, k);
	printf("Matrix m2:\n");
	doubleMatrixPrint(m2, k, 1);
	
	n_thread_left = k;
	pthread_mutex_init(&n_thread_mutex, NULL);
	pthread_t *threads = malloc(k * sizeof(pthread_t));
	for (i = 0; i < k; i++) {
		int *param = malloc(sizeof(int));
		*param = i;
		retval = pthread_create(&threads[i], NULL, thread_function, param);
		if (retval != 0) {
			printf("Error creating thread");
			exit(-1);
		}
		pthread_detach(threads[i]);
	}
	/*
	for (i = 0; i < k; i++) {
		pthread_join(threads[i], NULL);
	}
	pthread_mutex_destroy(&n_thread_mutex);
	*/
	pthread_exit(0);
	return 0;
}

void *thread_function(void *param) {
	int i = *(int *)param;
	int j;
	// calculate the i-th element
	for (j = 0; j < k; j++) {
		v[i] += mat[i][j] * v2[j];
	}
	
	// this critical section is executed in mutual exclusion
	pthread_mutex_lock(&n_thread_mutex);
	int tmp = --n_thread_left;
	pthread_mutex_unlock(&n_thread_mutex);
	// end of critical section
	
	// print for debug purpose
	//printf("v[%d] = %f\tn_thread_left = %d\n", i, v[i], tmp);
	fflush(stdout);
	
	
	if (tmp == 0) {
		// this is the last thread
		result = 0;
		for (j = 0; j < k; j++) {
			result += v1[j] * v[j];
		}
		
		printf("The result is: %f\n", result);
	}
	return NULL;
}

double **doubleVettToVerticalMatrix(double *vett, int dim) {
	int i;
	double **mat = malloc(dim * sizeof(double *));
	for (i = 0; i < dim; i++) {
		mat[i] = malloc(dim * sizeof(double));
		mat[i][0] = vett[i];
	}
	return mat;
}

double **doubleMatrixTranspone(double **mat, int m, int n) {
	int i, j;
	double **result = malloc(n * sizeof(double *));
	for (i = 0; i < n; i++) {
		result[i] = malloc(m * sizeof(double));
		for (j = 0; j < m; j++) {
			result[i][j] = mat[j][i];
		}
	}
	return result;
}

void doubleMatrixPrint(double ** mat, int m, int n) {
	int i, j;
	for (i = 0; i < m; i++) {
		for (j = 0; j < n; j++) {
			printf("%f\t", mat[i][j]);
		}
		printf("\n");
	}
}
