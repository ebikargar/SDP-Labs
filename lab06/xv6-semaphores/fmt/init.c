7800 // init: The initial user-level program
7801 
7802 #include "types.h"
7803 #include "stat.h"
7804 #include "user.h"
7805 #include "fcntl.h"
7806 
7807 char *argv[] = { "sh", 0 };
7808 
7809 int
7810 main(void)
7811 {
7812   int pid, wpid;
7813 
7814   if(open("console", O_RDWR) < 0){
7815     mknod("console", 1, 1);
7816     open("console", O_RDWR);
7817   }
7818   dup(0);  // stdout
7819   dup(0);  // stderr
7820 
7821   for(;;){
7822     printf(1, "init: starting sh\n");
7823     pid = fork();
7824     if(pid < 0){
7825       printf(1, "init: fork failed\n");
7826       exit();
7827     }
7828     if(pid == 0){
7829       exec("sh", argv);
7830       printf(1, "init: exec sh failed\n");
7831       exit();
7832     }
7833     while((wpid=wait()) >= 0 && wpid != pid)
7834       printf(1, "zombie!\n");
7835   }
7836 }
7837 
7838 
7839 
7840 
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
