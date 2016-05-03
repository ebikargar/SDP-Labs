#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>

#define BUFFER_LENGTH 100   
static char receive[BUFFER_LENGTH]; 

int main(int argc, char ** argv){
   int ret, fd;
   char buf[BUFFER_LENGTH];
   
   if (argc < 2){
	   fprintf(stderr, "Syntax: %s /dev/chardev..\n", argv[0]);
	   return -1;
   }
   fd = open(argv[1], O_RDWR); 
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }
   printf("Type in a short string (< %d characters):\n", BUFFER_LENGTH);
   fgets(buf, sizeof(buf), stdin); 
   ret = write(fd, buf, strlen(buf));
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }

   printf("Press ENTER to read back from the device\n");
   getchar();

   printf("Reading from the device\n");
   ret = read(fd, receive, BUFFER_LENGTH);
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("Read message: %s\n", receive);
   return 0;
}
