#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define MAX_NUMBER 1000


int main(int argc, char **argv) {
    time_t t;
    int n, i;
    int *vett;
    FILE *fp;
    
    // check the number of parameters
    if (argc < 3) {
        printf("Syntax: %s number_of_integers filename_output\n", argv[0]);
        return -1;
    }
    n = atoi(argv[1]);
    vett = malloc(n * sizeof(int));
    if(vett == NULL) {
        printf("Error allocating memory!\n");
        return -1;
    }
    // initialize random seed
    srand((unsigned) time(&t));
    // fill with random values in the range
    for(i = 0; i < n; i++) {
        vett[i] = rand() % MAX_NUMBER;
    }
    
    // write binary files
    fp = fopen(argv[2], "wb");
    if(fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }
    fwrite(vett, sizeof(int), n, fp);
    fclose(fp);
    
    printf("Terminated successfully!\nResult is stored in %s\n", argv[2]);
    return 0;
}
