#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
  int left;
  int right;
} Region;

int len, n, *v;


static void *
qsorter (void *arg) {
	Region *region = (Region *) arg;
	int left = region->left;
	int right = region->right;
	int i, j, x, tmp;
  pthread_t th1, th2;
  void *retval;
  fprintf(stderr, "New thread\n");
	
  if (left >= right)
    return NULL;

  x = v[left];
  i = left - 1;
  j = right + 1;

  while (i < j) {
    while (v[--j] > x);
    while (v[++i] < x);
    if (i < j)	{
	    tmp = v[i];
      v[i] = v[j];
      v[j] = tmp;
    }
  }
  region = (Region *) malloc (sizeof(Region));
  region->left = left;
  region->right = j;
  pthread_create (&th1, NULL, qsorter, region);
  
  region = (Region *) malloc (sizeof(Region));
  region->left = j+1;
  region->right = right;
  pthread_create (&th2, NULL, qsorter, region);

  pthread_join (th1, &retval); 
  pthread_join (th2, &retval); 
  
  return 0;
}


int
main (int argc, char ** argv)
{
  pthread_t th;
  int i;
  struct stat stat_buf;
  int fd;
  void *retval; 
  Region *region;
  char *paddr;
  
  if (argc != 2) {
    printf ("Syntax: %s filename\n", argv[0]);
    return (1);
  }
  
  if ((fd = open (argv[1], O_RDWR)) == -1)
    perror ("open");

  if (fstat (fd, &stat_buf) == -1)
    perror ("fstat");
    //size of the file
  len = stat_buf.st_size;
  n = len / sizeof (int);

  paddr = mmap ((caddr_t) 0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (paddr == (caddr_t) - 1)
    perror ("mmap");

  close (fd);
  // now we have a vector in virtual memory (swap partition)
  
  v = (int *) paddr;
  
  region = (Region *) malloc (sizeof(Region));
  region->left = 0;
  region->right = n-1;
  pthread_create (&th, NULL, qsorter, region);
  pthread_join (th, &retval);  

// writes are on the disk
  for(i=0;i<n;i++)
    printf("%d\n", v[i]); 
  
  return 0;
}


