#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

//#include <pthread.h>

double **doubleMatrixMult(double **m1, double **m2, int p, int q, int r);
double **doubleVettToVerticalMatrix(double *vett, int dim);
double **doubleMatrixTranspone(double **mat, int m, int n);
void doubleMatrixPrint(double **mat, int m, int n);

int main(int argc, char **argv) {

	int k;
	double *v1, *v2;
	double **mat;
	double **m1, **m2, **m1t, **intermediate, **resultMat;
	double result;

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

	for (i = 0; i < k; i++) {
		v1[i] = ((double)rand()) / INT_MAX - 0.5;
		v2[i] = ((double)rand()) / INT_MAX - 0.5;
		mat[i] = malloc(k * sizeof(double));
		for (j = 0; j < k; j++) {
			mat[i][j] = ((double)rand()) / INT_MAX - 0.5;
		}
	}

	intermediate = malloc(k * sizeof(double *));

	m1 = doubleVettToVerticalMatrix(v1, k);
	m1t = doubleMatrixTranspone(m1, k, 1);
	printf("Matrix m1t:\n");
	doubleMatrixPrint(m1t, 1, k);
	printf("Matrix mat:\n");
	doubleMatrixPrint(mat, k, k);
	m2 = doubleVettToVerticalMatrix(v2, k);
	printf("Matrix m2:\n");
	doubleMatrixPrint(m2, k, 1);
	
	intermediate = doubleMatrixMult(mat, m2, k, k, 1);
	//doubleMatrixPrint(intermediate, k, 1);
	resultMat = doubleMatrixMult(m1t, intermediate, 1, k, 1);
	result = resultMat[0][0];
	printf("Result is: %f\n", result);

	return 0;
}

double **doubleMatrixMult(double **m1, double **m2, int p, int q, int r) {
	// m1 is p x q, m2 is q x r
	double ** result = malloc(p * sizeof(double *));
	int i, j, k;
	// for each line of m1
	for (i = 0; i < p; i++) {
		result[i] = malloc(r * sizeof(double));
		// for each column of m2
		for (j = 0; j < r; j++) {
			double curr = 0;
			// calculate line * column
			for (k = 0; k < q; k++) {
				curr += m1[i][k] * m2[k][j];
			}
			result[i][j] = curr;
		}
	}
	return result;
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