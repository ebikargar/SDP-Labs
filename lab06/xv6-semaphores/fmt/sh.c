7850 // Shell.
7851 
7852 #include "types.h"
7853 #include "user.h"
7854 #include "fcntl.h"
7855 
7856 // Parsed command representation
7857 #define EXEC  1
7858 #define REDIR 2
7859 #define PIPE  3
7860 #define LIST  4
7861 #define BACK  5
7862 
7863 #define MAXARGS 10
7864 
7865 struct cmd {
7866   int type;
7867 };
7868 
7869 struct execcmd {
7870   int type;
7871   char *argv[MAXARGS];
7872   char *eargv[MAXARGS];
7873 };
7874 
7875 struct redircmd {
7876   int type;
7877   struct cmd *cmd;
7878   char *file;
7879   char *efile;
7880   int mode;
7881   int fd;
7882 };
7883 
7884 struct pipecmd {
7885   int type;
7886   struct cmd *left;
7887   struct cmd *right;
7888 };
7889 
7890 struct listcmd {
7891   int type;
7892   struct cmd *left;
7893   struct cmd *right;
7894 };
7895 
7896 struct backcmd {
7897   int type;
7898   struct cmd *cmd;
7899 };
7900 int fork1(void);  // Fork but panics on failure.
7901 void panic(char*);
7902 struct cmd *parsecmd(char*);
7903 
7904 // Execute cmd.  Never returns.
7905 void
7906 runcmd(struct cmd *cmd)
7907 {
7908   int p[2];
7909   struct backcmd *bcmd;
7910   struct execcmd *ecmd;
7911   struct listcmd *lcmd;
7912   struct pipecmd *pcmd;
7913   struct redircmd *rcmd;
7914 
7915   if(cmd == 0)
7916     exit();
7917 
7918   switch(cmd->type){
7919   default:
7920     panic("runcmd");
7921 
7922   case EXEC:
7923     ecmd = (struct execcmd*)cmd;
7924     if(ecmd->argv[0] == 0)
7925       exit();
7926     exec(ecmd->argv[0], ecmd->argv);
7927     printf(2, "exec %s failed\n", ecmd->argv[0]);
7928     break;
7929 
7930   case REDIR:
7931     rcmd = (struct redircmd*)cmd;
7932     close(rcmd->fd);
7933     if(open(rcmd->file, rcmd->mode) < 0){
7934       printf(2, "open %s failed\n", rcmd->file);
7935       exit();
7936     }
7937     runcmd(rcmd->cmd);
7938     break;
7939 
7940   case LIST:
7941     lcmd = (struct listcmd*)cmd;
7942     if(fork1() == 0)
7943       runcmd(lcmd->left);
7944     wait();
7945     runcmd(lcmd->right);
7946     break;
7947 
7948 
7949 
7950   case PIPE:
7951     pcmd = (struct pipecmd*)cmd;
7952     if(pipe(p) < 0)
7953       panic("pipe");
7954     if(fork1() == 0){
7955       close(1);
7956       dup(p[1]);
7957       close(p[0]);
7958       close(p[1]);
7959       runcmd(pcmd->left);
7960     }
7961     if(fork1() == 0){
7962       close(0);
7963       dup(p[0]);
7964       close(p[0]);
7965       close(p[1]);
7966       runcmd(pcmd->right);
7967     }
7968     close(p[0]);
7969     close(p[1]);
7970     wait();
7971     wait();
7972     break;
7973 
7974   case BACK:
7975     bcmd = (struct backcmd*)cmd;
7976     if(fork1() == 0)
7977       runcmd(bcmd->cmd);
7978     break;
7979   }
7980   exit();
7981 }
7982 
7983 int
7984 getcmd(char *buf, int nbuf)
7985 {
7986   printf(2, "$ ");
7987   memset(buf, 0, nbuf);
7988   gets(buf, nbuf);
7989   if(buf[0] == 0) // EOF
7990     return -1;
7991   return 0;
7992 }
7993 
7994 
7995 
7996 
7997 
7998 
7999 
8000 int
8001 main(void)
8002 {
8003   static char buf[100];
8004   int fd;
8005 
8006   // Assumes three file descriptors open.
8007   while((fd = open("console", O_RDWR)) >= 0){
8008     if(fd >= 3){
8009       close(fd);
8010       break;
8011     }
8012   }
8013 
8014   // Read and run input commands.
8015   while(getcmd(buf, sizeof(buf)) >= 0){
8016     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
8017       // Clumsy but will have to do for now.
8018       // Chdir has no effect on the parent if run in the child.
8019       buf[strlen(buf)-1] = 0;  // chop \n
8020       if(chdir(buf+3) < 0)
8021         printf(2, "cannot cd %s\n", buf+3);
8022       continue;
8023     }
8024     if(fork1() == 0)
8025       runcmd(parsecmd(buf));
8026     wait();
8027   }
8028   exit();
8029 }
8030 
8031 void
8032 panic(char *s)
8033 {
8034   printf(2, "%s\n", s);
8035   exit();
8036 }
8037 
8038 int
8039 fork1(void)
8040 {
8041   int pid;
8042 
8043   pid = fork();
8044   if(pid == -1)
8045     panic("fork");
8046   return pid;
8047 }
8048 
8049 
8050 // Constructors
8051 
8052 struct cmd*
8053 execcmd(void)
8054 {
8055   struct execcmd *cmd;
8056 
8057   cmd = malloc(sizeof(*cmd));
8058   memset(cmd, 0, sizeof(*cmd));
8059   cmd->type = EXEC;
8060   return (struct cmd*)cmd;
8061 }
8062 
8063 struct cmd*
8064 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
8065 {
8066   struct redircmd *cmd;
8067 
8068   cmd = malloc(sizeof(*cmd));
8069   memset(cmd, 0, sizeof(*cmd));
8070   cmd->type = REDIR;
8071   cmd->cmd = subcmd;
8072   cmd->file = file;
8073   cmd->efile = efile;
8074   cmd->mode = mode;
8075   cmd->fd = fd;
8076   return (struct cmd*)cmd;
8077 }
8078 
8079 struct cmd*
8080 pipecmd(struct cmd *left, struct cmd *right)
8081 {
8082   struct pipecmd *cmd;
8083 
8084   cmd = malloc(sizeof(*cmd));
8085   memset(cmd, 0, sizeof(*cmd));
8086   cmd->type = PIPE;
8087   cmd->left = left;
8088   cmd->right = right;
8089   return (struct cmd*)cmd;
8090 }
8091 
8092 
8093 
8094 
8095 
8096 
8097 
8098 
8099 
8100 struct cmd*
8101 listcmd(struct cmd *left, struct cmd *right)
8102 {
8103   struct listcmd *cmd;
8104 
8105   cmd = malloc(sizeof(*cmd));
8106   memset(cmd, 0, sizeof(*cmd));
8107   cmd->type = LIST;
8108   cmd->left = left;
8109   cmd->right = right;
8110   return (struct cmd*)cmd;
8111 }
8112 
8113 struct cmd*
8114 backcmd(struct cmd *subcmd)
8115 {
8116   struct backcmd *cmd;
8117 
8118   cmd = malloc(sizeof(*cmd));
8119   memset(cmd, 0, sizeof(*cmd));
8120   cmd->type = BACK;
8121   cmd->cmd = subcmd;
8122   return (struct cmd*)cmd;
8123 }
8124 
8125 
8126 
8127 
8128 
8129 
8130 
8131 
8132 
8133 
8134 
8135 
8136 
8137 
8138 
8139 
8140 
8141 
8142 
8143 
8144 
8145 
8146 
8147 
8148 
8149 
8150 // Parsing
8151 
8152 char whitespace[] = " \t\r\n\v";
8153 char symbols[] = "<|>&;()";
8154 
8155 int
8156 gettoken(char **ps, char *es, char **q, char **eq)
8157 {
8158   char *s;
8159   int ret;
8160 
8161   s = *ps;
8162   while(s < es && strchr(whitespace, *s))
8163     s++;
8164   if(q)
8165     *q = s;
8166   ret = *s;
8167   switch(*s){
8168   case 0:
8169     break;
8170   case '|':
8171   case '(':
8172   case ')':
8173   case ';':
8174   case '&':
8175   case '<':
8176     s++;
8177     break;
8178   case '>':
8179     s++;
8180     if(*s == '>'){
8181       ret = '+';
8182       s++;
8183     }
8184     break;
8185   default:
8186     ret = 'a';
8187     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
8188       s++;
8189     break;
8190   }
8191   if(eq)
8192     *eq = s;
8193 
8194   while(s < es && strchr(whitespace, *s))
8195     s++;
8196   *ps = s;
8197   return ret;
8198 }
8199 
8200 int
8201 peek(char **ps, char *es, char *toks)
8202 {
8203   char *s;
8204 
8205   s = *ps;
8206   while(s < es && strchr(whitespace, *s))
8207     s++;
8208   *ps = s;
8209   return *s && strchr(toks, *s);
8210 }
8211 
8212 struct cmd *parseline(char**, char*);
8213 struct cmd *parsepipe(char**, char*);
8214 struct cmd *parseexec(char**, char*);
8215 struct cmd *nulterminate(struct cmd*);
8216 
8217 struct cmd*
8218 parsecmd(char *s)
8219 {
8220   char *es;
8221   struct cmd *cmd;
8222 
8223   es = s + strlen(s);
8224   cmd = parseline(&s, es);
8225   peek(&s, es, "");
8226   if(s != es){
8227     printf(2, "leftovers: %s\n", s);
8228     panic("syntax");
8229   }
8230   nulterminate(cmd);
8231   return cmd;
8232 }
8233 
8234 struct cmd*
8235 parseline(char **ps, char *es)
8236 {
8237   struct cmd *cmd;
8238 
8239   cmd = parsepipe(ps, es);
8240   while(peek(ps, es, "&")){
8241     gettoken(ps, es, 0, 0);
8242     cmd = backcmd(cmd);
8243   }
8244   if(peek(ps, es, ";")){
8245     gettoken(ps, es, 0, 0);
8246     cmd = listcmd(cmd, parseline(ps, es));
8247   }
8248   return cmd;
8249 }
8250 struct cmd*
8251 parsepipe(char **ps, char *es)
8252 {
8253   struct cmd *cmd;
8254 
8255   cmd = parseexec(ps, es);
8256   if(peek(ps, es, "|")){
8257     gettoken(ps, es, 0, 0);
8258     cmd = pipecmd(cmd, parsepipe(ps, es));
8259   }
8260   return cmd;
8261 }
8262 
8263 struct cmd*
8264 parseredirs(struct cmd *cmd, char **ps, char *es)
8265 {
8266   int tok;
8267   char *q, *eq;
8268 
8269   while(peek(ps, es, "<>")){
8270     tok = gettoken(ps, es, 0, 0);
8271     if(gettoken(ps, es, &q, &eq) != 'a')
8272       panic("missing file for redirection");
8273     switch(tok){
8274     case '<':
8275       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
8276       break;
8277     case '>':
8278       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8279       break;
8280     case '+':  // >>
8281       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8282       break;
8283     }
8284   }
8285   return cmd;
8286 }
8287 
8288 
8289 
8290 
8291 
8292 
8293 
8294 
8295 
8296 
8297 
8298 
8299 
8300 struct cmd*
8301 parseblock(char **ps, char *es)
8302 {
8303   struct cmd *cmd;
8304 
8305   if(!peek(ps, es, "("))
8306     panic("parseblock");
8307   gettoken(ps, es, 0, 0);
8308   cmd = parseline(ps, es);
8309   if(!peek(ps, es, ")"))
8310     panic("syntax - missing )");
8311   gettoken(ps, es, 0, 0);
8312   cmd = parseredirs(cmd, ps, es);
8313   return cmd;
8314 }
8315 
8316 struct cmd*
8317 parseexec(char **ps, char *es)
8318 {
8319   char *q, *eq;
8320   int tok, argc;
8321   struct execcmd *cmd;
8322   struct cmd *ret;
8323 
8324   if(peek(ps, es, "("))
8325     return parseblock(ps, es);
8326 
8327   ret = execcmd();
8328   cmd = (struct execcmd*)ret;
8329 
8330   argc = 0;
8331   ret = parseredirs(ret, ps, es);
8332   while(!peek(ps, es, "|)&;")){
8333     if((tok=gettoken(ps, es, &q, &eq)) == 0)
8334       break;
8335     if(tok != 'a')
8336       panic("syntax");
8337     cmd->argv[argc] = q;
8338     cmd->eargv[argc] = eq;
8339     argc++;
8340     if(argc >= MAXARGS)
8341       panic("too many args");
8342     ret = parseredirs(ret, ps, es);
8343   }
8344   cmd->argv[argc] = 0;
8345   cmd->eargv[argc] = 0;
8346   return ret;
8347 }
8348 
8349 
8350 // NUL-terminate all the counted strings.
8351 struct cmd*
8352 nulterminate(struct cmd *cmd)
8353 {
8354   int i;
8355   struct backcmd *bcmd;
8356   struct execcmd *ecmd;
8357   struct listcmd *lcmd;
8358   struct pipecmd *pcmd;
8359   struct redircmd *rcmd;
8360 
8361   if(cmd == 0)
8362     return 0;
8363 
8364   switch(cmd->type){
8365   case EXEC:
8366     ecmd = (struct execcmd*)cmd;
8367     for(i=0; ecmd->argv[i]; i++)
8368       *ecmd->eargv[i] = 0;
8369     break;
8370 
8371   case REDIR:
8372     rcmd = (struct redircmd*)cmd;
8373     nulterminate(rcmd->cmd);
8374     *rcmd->efile = 0;
8375     break;
8376 
8377   case PIPE:
8378     pcmd = (struct pipecmd*)cmd;
8379     nulterminate(pcmd->left);
8380     nulterminate(pcmd->right);
8381     break;
8382 
8383   case LIST:
8384     lcmd = (struct listcmd*)cmd;
8385     nulterminate(lcmd->left);
8386     nulterminate(lcmd->right);
8387     break;
8388 
8389   case BACK:
8390     bcmd = (struct backcmd*)cmd;
8391     nulterminate(bcmd->cmd);
8392     break;
8393   }
8394   return cmd;
8395 }
8396 
8397 
8398 
8399 
