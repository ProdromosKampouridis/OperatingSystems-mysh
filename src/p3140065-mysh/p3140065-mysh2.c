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
#define prompt "mysh2"



// ##############################
int main (int argc, char **argv) {
	char input[BUFSIZE];
	
	while(!feof(stdin)) {
		// show the prompt
		printf("%s> ",prompt);
		input[0]='\0';
		if ( fgets(input, sizeof(input)-1, stdin) == NULL ) {
   			puts("EOF detected\n");
			exit(0);
		}
		// overwrite the new line character
		input[strlen(input)-1]='\0';
		//		printf("I read : '%s'\n",input);
		int status=fork_and_wait(input);
		if ( status == -1  ) {	
			printf("Cound not spawn command %s\n",input);
		}
	}
}



// ##############################
int fork_and_wait(char *input) {
	pid_t child_pid;
	int w,status;
    const char *command ;

	// the param separator is the space
	const char s[1] = " ";
	char *token;
   

	//fork a duplicate process
	child_pid = fork();
	if ( child_pid == -1 ) {
		return(-1);
	}

	//if the current process is a child of the main process
	if (child_pid == 0) {
		//here I need to execute whatever program was given to user_input
		exec_cmd(input);
		// if exec succeeds this line is never executed, if not die die die
    	exit(0);
	}
	// I am the parent , so I wait
    w=waitpid(child_pid,&status,0);
	return(w);
}
