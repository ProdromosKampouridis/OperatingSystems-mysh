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
#define prompt "mysh4"



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
// ##############################
int fork_and_wait(char *input) {
    char execs[ARGCOUNT][BUFSIZE];	// up to 2 executables in the pipeline
	int fd[2];			// file descriptors for pipe
    const char *command1;	
    const char *command2;	
	char *token;		// used in breaking down strings
	int inredir=0;
	int outredir=0;
	int outappend=0;
	char saveinput[BUFSIZE];
	char single_exec[BUFSIZE];
	char infile[BUFSIZE];
	char outfile[BUFSIZE];
	FILE *in,*out;
	int childcount=0;
	int numexecs=0;
	int wehavepipes=0;
	pid_t pid,childpid[10]; // up to 10 children

	infile[0]	='\0';
	outfile[0]	='\0';

	findioredir(input,infile,outfile,&inredir,&outredir,&outappend);
	// we now have the input redirect the output redirect and we have the rest of the  command line clean from them

	int pipecount=0;	
	// tokenize the input
	// tokenization proper , input now contains just the executable and its parameters
	token = strtok(input, "|" );
	strcpy(execs[0],token);		// the command is the whole string
	token = strtok(NULL, "|");
	if ( token != NULL ) {
		wehavepipes=1;
		inredir=0;		// cannot do input redirection with pipes
		strcpy(execs[1],token);	// each element of the new argv
		numexecs++;
	}
	
	
   
	//our pipes
	// printf("execs[0] %s execs[1] %s pipes:%d\n",execs[0],execs[1],wehavepipes);
	   

		// we now have executable, params and redirection
		int i=0;
		
		int returnValue = pipe(fd);
		if (returnValue == -1) {
			printf("ERROR: Pipe command failed.\n");
			return -1;
		}	
		//fork a duplicate process
		pid = fork();
		if ( pid == -1 ) {	// could not spawn
			return(-1);
		}

		if ( pid == 0 ) { //if the current process is a child of the main process
			if ( wehavepipes==1) {
				dup2(fd[1],fileno(stdout));	// first command stdout is next command stdin
				close(fd[0]);
				close(fd[1]);
			} else {
				ioredir(infile,outfile,inredir,outredir,outappend); // Warning input redirection will break multiple pipes
			}
			exec_cmd(execs[0]);
		}
		childpid[childcount++]=pid;

		// the second (last) command	
		if ( numexecs >= 1) { // it also means wehavepipes=1

			//fork a duplicate process
			int xpid = fork();
			if ( xpid == -1 ) {	// could not spawn
				waitall(childcount,childpid);
				return(-1);
			}
	
			if ( xpid == 0 ) { //if the current process is a child of the main process
				dup2(fd[0],fileno(stdin));	
				close(fd[0]);
				close(fd[1]);
				ioredir(infile,outfile,inredir,outredir,outappend); // Warning input redirection will break multiple pipes
				exec_cmd(execs[1]);		
			}
			childpid[childcount++]=xpid;
		}

	close(fd[0]);
	close(fd[1]);
	waitall(childcount,childpid); // no harm
	return(1);
}


