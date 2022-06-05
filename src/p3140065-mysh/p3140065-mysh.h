
#define BUFSIZE 1000
#define ARGCOUNT 10

int fork_and_wait(char *command);


int makeargv(const char *command,char argv[ARGCOUNT][BUFSIZE]);
void ioredir(char *infile,char *outfile,int inredir,int outredir,int outappend);
void findioredir(char *input,char *infile,char *outfile,int *inredir,int *outredir,int *outappend);
void exec_cmd(char *exec);
void waitall(int childcount,int childpid[]);
void closepipes(int fd[][2]);

