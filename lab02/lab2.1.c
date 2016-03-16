#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

typedef struct {
  int *vet;
  int left;
  int right;
} mergeParamType;

void 
merge(int *vet, int left, int middle, int right) {
    int i, j, k;
    int n1 = middle - left + 1;
    int n2 =  right - middle;

    int L[n1], R[n2];  // temp arrays
    
    for (i = 0; i <= n1; i++)  // make a copy
        L[i] = vet[left + i];
    for (j = 0; j <= n2; j++)
        R[j] = vet[middle + 1 + j];
        
    // Merge the temp arrays in vet[l..r]
    i = 0; 
    j = 0; 
    k = left; // Initial index of merged subarray
    while (i < n1 && j < n2)  {
      if (L[i] <= R[j])
        vet[k++] = L[i++];
      else
        vet[k++] = R[j++];
    }
    // Copy the remaining elements of L[]
    while (i < n1)
      vet[k++] = L[i++];
    // Copy the remaining elements of R[]
    while (j < n2)
      vet[k++] = R[j++];
}
 
void *mergeSort(void *param){
  // parameters for the two threads
  mergeParamType lp, rp;
  mergeParamType *p = (mergeParamType *) param;
  int middle;
  //printf("mergeSort: %d %d\n", p->left, p->right);
  
  pthread_t leftThread, rightThread;
  int retcode_left, retcode_right;

  if (p->left < p->right){
    middle = p->left + (p->right - p->left)/2;  // Same as (left + right)/2, but avoids overflow for large l and r
    //printf("middle = %d\n", middle);
 	// prepare parameters for left and right recursion
   lp.vet = p->vet;
   lp.left = p->left;
   lp.right = middle;
   
   rp.vet = p->vet;
   rp.left = middle+1;
   rp.right = p->right;
   
   // create threads
   retcode_left = pthread_create(&leftThread, NULL, mergeSort, (void *) &lp);
   retcode_right = pthread_create(&rightThread, NULL, mergeSort, (void *) &rp);
   if (retcode_left != 0 || retcode_right != 0) {
     printf("Error creating a thread\n");
     exit(1);
   }
    //mergeSort(vet, left, middle);
    //mergeSort(vet, middle+1, right);
 	// join with threads
   pthread_join(leftThread, NULL);
   pthread_join(rightThread, NULL);
    merge(p->vet, p->left, middle, p->right);
  }
  return NULL;
}

int main(int argc, char ** argv) {
  int i, n;
  int *vet;
  
  mergeParamType param;
  //pthread_t sortThread;
  
  if (argc != 2) {
    printf ("Syntax: %s dimension\n", argv[0]);
    return (1);
  }
  
  n = atoi(argv[1]);

  vet = (int*) malloc(n * sizeof(int));
  
  srand(n);
  
  for(i = 0;i < n;i++) {
    vet[i] = rand() % 100;
	 printf("%d\n",vet[i]);
  }
	
  param.vet = vet;
  param.left = 0;
  param.right = n-1;
  
  //pthread_create(&sortThread, NULL, mergeSort, (void *) &param);
  //pthread_join(sortThread, NULL);
  //mergeSort(vet,0, n-1);
  mergeSort(&param);
  printf("\n");
  for(i = 0;i < n;i++) 
	printf("%d\n",vet[i]);
	
  return 0;
}
