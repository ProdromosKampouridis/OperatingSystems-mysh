// input ouput
#include <stdio.h>
// string operations
#include <string.h>
//scanf stuff
#include <stdarg.h>
#include <stdlib.h>
// fork & wait
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


// my stuff
#include "p314-p300-p365-mysh.h"
#define prompt "mysh5"


// ##############################
int main (int argc, char **argv) {
	char input[BUFSIZE];
	int save_in,save_out;

	save_in=dup(fileno(stdin));
	save_out=dup(fileno(stdout));
	
	while(!feof(stdin)) {
		// show the prompt
		printf("\n%s> ",prompt);
		input[0]='\0';
		if ( fgets(input, sizeof(input)-1, stdin) == NULL ) {
   			puts("EOF detected\n");
			exit(0);
		}
		// overwrite the new line character
		input[strlen(input)-1]='\0';
		//		printf("I read : '%s'\n",input);
		if (input[0]!='\0') {
			int status=fork_and_wait(input);
			if ( status == -1  ) {	
				printf("Cound not spawn command %s\n",input);
			}
			input[0]='\0'; // zero out the input
		}
		dup2(save_in,fileno(stdin));
		dup2(save_out,fileno(stdout));
	}
}



// ##############################
// the main program is really here
// ##############################
int fork_and_wait(char *input) {
    char execs[ARGCOUNT][BUFSIZE];	// up to 2 executables in the pipeline
	char *token;		// used in breaking down strings
	int inredir=0;
	int outredir=0;
	int outappend=0;
	char saveinput[BUFSIZE];
	char infile[BUFSIZE];
	char outfile[BUFSIZE];
	FILE *in,*out;
	int childcount=0;
	int numexecs=0;
	int wehavepipes=0,i=0;
	pid_t pid;
	pid_t childpid[ARGCOUNT]; // up to 10 children
	int fd[ARGCOUNT][2]; // up to 10 pipes

	infile[0]	='\0';
	outfile[0]	='\0';

	strcpy(saveinput,input); // just in case
	findioredir(input,infile,outfile,&inredir,&outredir,&outappend);
	// we now have the input redirect the output redirect and we have the rest of the  command line clean from them

	int pipecount=0;	
	// tokenize the input
	// tokenization proper , input now contains just the executable and its parameters
	token = strtok(input, "|" );
	strcpy(execs[0],token);		// the command is the whole string
	numexecs++;					//we have at least one executable
	while ( (token != NULL ) && (numexecs < ARGCOUNT) ){	// we can only handle up to 10 executables
		token = strtok(NULL, "|");
		if ( token != NULL ) {
			wehavepipes=1;
			inredir=0;		// cannot do input redirection with pipes
			strcpy(execs[numexecs],token);	// each element of the new argv
			numexecs++;
		}
	}
	
	//our pipes
	//  for (i=0;i<numexecs; i++)  {
	//	printf("execs[%d] = %s\n",i,execs[i]);
	// }

	// we now have executable, params and redirection

	//first create all the pipes
	for (i=0; i<ARGCOUNT; i++ ) {
		int returnValue = pipe(fd[i]);
		if (returnValue == -1) {
			printf("ERROR: Pipe command failed.\n");
			closepipes(fd);	// any remaining open ones
			return -1;
		}	
	}

	i=0;
	//fork a duplicate process
	pid = fork();
	if ( pid == -1 ) {	// could not spawn
		return(-1);
	}

	if ( pid == 0 ) { //if the current process is a child of the main process
		if ( wehavepipes==1) {
			dup2(fd[0][1],fileno(stdout));	// first command stdout is next command stdin
			closepipes(fd);
		} else {
			ioredir(infile,outfile,inredir,outredir,outappend); // Warning input redirection will break multiple pipes
		}
		exec_cmd(execs[0]);
	} else {
		childpid[childcount++]=pid;
	}


	// fork all the intermediate execs
	// except the last one which will show stdout to the terminal
	for (i=1;i<numexecs-1;i++) {
		//fprintf(stderr, "In pipe fork %d wehavepipes %d\n",i,wehavepipes);
		//fork a duplicate process
		pid = fork();
		if ( pid == -1 ) {	// could not spawn
			waitall(childcount,childpid);
			return(-1);
		}

		if ( pid == 0 ) { //if the current process is a child of the main process
			if ( wehavepipes==1) {
				dup2(fd[i-1][0],fileno(stdin));	// stdin is previous pipes's stdin
				dup2(fd[i][1],fileno(stdout));	// current command's stdout is curren't pipe's stdout
				closepipes(fd);
			} 
			exec_cmd(execs[i]);
		} else {
			childpid[childcount++]=pid;
		}
	}

	// if pipes were used in the command line, this is the last command
	if ( wehavepipes == 1 ) { // last fork to pick up the output from the previous fork
		// the last in the row
		//fprintf(stderr, "last  fork %d wehavepipes %d program %s\n",numexecs,wehavepipes,execs[numexecs-1]);
		// the executable is at numexecs -1 
		// the output is at fd[numexecs-2]
		// maybe if I used [i] ?

		//fork a duplicate process
		pid = fork();
		if ( pid == -1 ) {	// could not spawn
			waitall(childcount,childpid);
			return(-1);
		}
		if ( pid == 0 ) { //if the current process is a child of the main process
			dup2(fd[numexecs-2][0],fileno(stdin));	// pick up any remaining input
			closepipes(fd);
			ioredir(infile,outfile,inredir,outredir,outappend); // Warning input redirection will break multiple pipes
			exec_cmd(execs[numexecs-1]);
		} else {
			childpid[childcount++]=pid;
		}
	}


	closepipes(fd);
	waitall(childcount,childpid);
	return(1);
}


