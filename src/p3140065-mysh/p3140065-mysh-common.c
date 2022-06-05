#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "p314-p300-p365-mysh.h"



//////////////////////////////////////////////////////////////////////
// populates newargv to be used by execlp with proper values
// to be used by exec_cmd
//////////////////////////////////////////////////////////////////////
int makeargv(const char *commandline,char newargv[ARGCOUNT][BUFSIZE]) {
		char single_exec[BUFSIZE];
		int count=0;

		strcpy(single_exec,commandline);

		// tokenize a single command line
		// tokenization proper , input now contains just the executable and its parameters
		char *token = strtok(single_exec, " " );
		newargv[0][0]='\0';
		strcpy(newargv[0],token);
	   
		/* walk through other tokens */
		int i=1;
		while ( ( token != NULL ) && ( i<ARGCOUNT))  {
			newargv[i][0]='\0';
			// printf( "tok: %s\n", token );
			token = strtok(NULL, " ");
			if ( token != NULL ) {
				strcpy(newargv[i],token);	// each element of the new argv
				i++;
			}
			count++;
		}
	return(count);
}


////////////////////////////////////////////////////////////////////////
// This was really difficult to find where infile is and outfile is
// it finds the infile and outfile in IO redirection
// it modifies input to get rid off anything after a > or a <
////////////////////////////////////////////////////////////////////////
void findioredir(char *input,char *infile,char *outfile,int *inredir,int *outredir, int *outappend) {
	char saveinput[BUFSIZE];
	strcpy(saveinput,input); // just in case
	// locate the position of the < character
	char *posin=strchr(input,'<');
	char *saveposin=posin;

	if ( posin != NULL ) { // I found an <
		*inredir=1;	// set the flag
		posin++;		// skip <
		while ( (posin[0]==' ') && (posin!=NULL) ) { // skip beginning spaces
			posin++;
		}
		// printf("posin:'%s'\n",posin);
	}

	// locate the position of the > character
	char *posout=strchr(input,'>');
	char *saveposout=posout;
	if ( posout != NULL ) { // I found an >
		*outredir=1;
		posout++;		// skip >
//		printf("firstposout:'%s'\n",posout);
	
		if ( posout[0] == '>')  {			// double >> located
			*outappend=1;	// we must append to the file
			posout++;		// skip another >
		}
		while ( (posout[0]==' ') && (posout!=NULL) ) { // skip beginning spaces
			posout++;
		}
//		printf("outappend:%d posout:'%s'\n",*outappend,posout);
	}


	// I had both input and output redirection so I have to break it down 
	if ( (posin != NULL) && (posout != NULL) ) {
		if (posout > posin ) {
			char *out=strchr(posin,'>');
			out[0]='\0';
		}
		if (posin > posout ) {
			char *pin=strchr(posout,'<');
			pin[0]='\0'; 
		}
	}

	if ( posin != NULL ) {
		strcpy(infile,posin);
		char *sp=strchr(infile,' ');
		if ( sp != NULL) { sp[0] = '\0';  } 	//clean extra spaces
	} 
	if ( posout != NULL ) {
		strcpy(outfile,posout);
		char *sp=strchr(outfile,' ');
		if ( sp != NULL) { sp[0] = '\0';  }	//clean extra spaces 
	}
//	printf("infile:'%s' outfile:'%s'\n",infile,outfile);
	strcpy(input,saveinput);
	// I don't  which redir comes first so I zero out both of them
	if (saveposin != NULL ) 	{ saveposin[0]='\0'; }	// if there is redirection get rid of it
	if (saveposout != NULL ) 	{ saveposout[0]='\0'; }	// if there is redirection get rid of it 

}


////////////////////////////////////////////////////////////////////////
// redirect stdout to file, 
// redirect stdin from file
////////////////////////////////////////////////////////////////////////
void ioredir(char *infile,char *outfile,int inredir,int outredir, int outappend) {
        // set input and output
        FILE *in,*out;
        if ( inredir == 1 ) {
            in=fopen(infile,"r");
            if ( in == NULL ) {
                printf("ERROR , input redir file not found:%s\n",infile);
                exit(1);
            }
            dup2(fileno(in),fileno(stdin)); // stdin come from infile
        }
        if ( outredir == 1 ) {
            if ( outappend==1) {
                out=fopen(outfile,"a");     // append
            } else {
                out=fopen(outfile,"w");     // just create
            }
            if ( out == NULL ) {
                printf("ERROR , ouput redir could not create file:%s\n",outfile);
                exit(1);
            }
            dup2(fileno(out),fileno(stdout));
        }

}

////////////////////////////////////////////////////////////////////////
// The actual execution
////////////////////////////////////////////////////////////////////////
void exec_cmd(char *exec){
    char newargv[ARGCOUNT][BUFSIZE]; // only up to 10 parameters per executable including its name
	char *execargv[ARGCOUNT] = { NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL};
	int i;

	// newarv is the actual storage of all the params and execargv just an array of pointers to keep execlp happy
	
//	fprintf(stderr,"About to fork:%s\n",exec);		

	int args=makeargv(exec,newargv);
//	 fprintf(stderr," args %d \n",args);
//	for (int i=0 ; i< args; i++ ) { fprintf(stderr,"exec:%s argv: %s\n",exec,newargv[i]); }

	// execvp requires a **Argv so I make one
	for (i=0 ; i< args; i++ ) {
		execargv[i]=&newargv[i][0];
	}
	// for (i=0 ; i< ARGCOUNT; i++ ) { fprintf(stderr,"exec:%s argv: %s\n",exec,execargv[i]); }

	int stat=execvp(execargv[0], execargv);
	fprintf(stderr,"stat is  %d\n",stat);
	if ( stat == -1 ) { 
		fprintf(stderr,"exec failed for %s\n",newargv[0]);
	}
	// if exec succeeds this line is never executed, if not die die die
	exit(stat);
}

////////////////////////////////////////////////////////////////////////
// Wait for all our children
////////////////////////////////////////////////////////////////////////
void waitall(int childcount, int childpid[]) {
	int status;
	int i;

	// I am the parent , so I wait for everyone
	for (i=0;i<childcount;i++) {
//		fprintf(stderr,"Waiting for pid:%d\n",childpid[i]);
    	waitpid(childpid[i],&status,0);
	}
}

////////////////////////////////////////////////////////////////////////
//   close all pipes
////////////////////////////////////////////////////////////////////////
void closepipes(int fd[][2]) {
	int i;
	for (i=0;i<ARGCOUNT;i++) {
		close(fd[i][0]);
		close(fd[i][1]);
	}
}
