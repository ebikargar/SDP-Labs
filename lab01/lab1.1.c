#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

void sort(int *vett, int dim);
void printError();

int main(int argc, char** argv) {
    int n1, n2, *v1, *v2, i;
    time_t t;
    FILE *fp;
	// check on number of parameters
    if(argc < 3) {
        printf("Please provide two integer parameters\n");
        return -1;
    }
    
    // parse integers
    if(sscanf(argv[1], "%d", &n1) != 1 || sscanf(argv[2], "%d", &n2) != 1) {
        printf("Please provide two integer parameters\n");
        return -1;
    }
    
    // allocate v1 and v2
    v1 = malloc(n1 * sizeof(int));
    v2 = malloc(n2 * sizeof(int));
    
    if(v1 == NULL || v2 == NULL) {
        printf("Error allocating resources\n");
        return -1;
    }

    //init random seed
    srand((unsigned) time(&t));

    for(i = 0; i < n1; i++) {
        // between 10 and 100
        v1[i] = rand() % 91 + 10;
    }
    for(i = 0; i < n2; i++) {
        // between 20 and 100
        v2[i] = rand() % 81 + 20;
    }
    
    // sort the two arrays
    sort(v1, n1);
    sort(v2, n2);

    // write textual files
    fp = fopen("fv1.txt", "wt");
    if(fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    for(i = 0; i < n1; i++) {
        fprintf(fp, "%d ", v1[i]);
    }
    fclose(fp);

    fp = fopen("fv2.txt", "wt");
    if(fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    for(i = 0; i < n2; i++) {
        fprintf(fp, "%d ", v2[i]);
    }
    fclose(fp);

    

    // write binary files
    fp = fopen("fv1.b", "wb");
    if(fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    fwrite(v1, sizeof(int), n1, fp);
    fclose(fp);

    fp = fopen("fv2.b", "wb");
    if(fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    fwrite(v2, sizeof(int), n2, fp);
    fclose(fp);

    return 0;
}


void sort(int *array, int n) {
    // perform a bubble sort
    int i, j, tmp;
    for (i = 0 ; i < (n - 1); i++) {
        for (j = 0 ; j < n - i - 1; j++) {
            if (array[j] > array[j+1]) {
                tmp = array[j];
                array[j] = array[j+1];
                array[j+1] = tmp;
            }
        }
    }
    return;
}
