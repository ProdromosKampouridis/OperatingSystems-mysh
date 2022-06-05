// ΠΡΟΔΡΟΜΟΣ ΚΑΜΠΟΥΡΙΔΗΣ ΑΜ:3140065
// please compile with
// gcc -o os_ex2 p3140065_os_ex2.c  -lpthread


// system includes
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>


#include "p3140065_os_ex2.h"


//the original numbers array
unsigned int numbers_ar[NUMBERS];
// used as an intermediate array for sorting
unsigned int merge_ar[NUMBERS];


// These variables are global because they are going to be used 
// by all the threads
int numbers=NUMBERS;
// default values
int threads=1;
int seed=5678;
int mode=1;

// ######### Thread specific stuff
// array of thread IDs to keep track of them
pthread_t tid[8]; // maximum 8 threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // the mutual exlusion bit part for accessing the array
pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER; // the mutual exlusion bit part for printing



int main(int argc, char **argv) {
	int i;
	int runlen;
	int start;
	int end;
	FILE *out;
	int pairs;
	int	starta;
	int	enda;
	int	startb;
	int	endb;
	int err;
	args_for_thread thread_args[8]; // arguments to each thread
	struct timeval t1,t2;
	double elapsedTime;

	gettimeofday(&t1,NULL);
	
	// ###################
	// ###################  command line section
	// ###################
	// Manage command line parameters
	// argc must be five
	if ( argc != 5 ) {
		printf("Usage: oe2 -numbers=<i> -threads=<i> -seed=<i> -mode=<i> \n");
		exit(1);
	}
	parse_args(argc,argv); // make sense of the command line
	printf("Numbers:%d Threads:%d Seed:%d Mode:%d\n",numbers,threads,seed,mode);
	// ########################################################################
	
	// ###################
	// ################### intialization section
	// ###################
	// seed the random number generator
	srand(seed);

	// initialize the numbers array
	for (i=0;i<numbers;i++) {
		numbers_ar[i]=rand() % 1000; // range up to 1000
	}
	
	// create the results file
	out=fopen("results.dat","w");
	if (out == NULL ) {
		printf("Could not create results file, exiting\n");
		exit(1);
	}
	// print the original data out
	fprintf(out,"Initial Array\n");
	fprintf(out,"=============\n");
	for (i=0;i<numbers;i++) {
		// print a line out to the file
		fprintf(out,"%d\n",numbers_ar[i]);
	}
	fprintf(out,"\n\n");
	// ###############################################################################
	
	// ###################
	// ################### threading section
	// ###################
	// how many elements of the numbers array will each thread sort ?
	// each thread will sort - runlen - numbers
	runlen=numbers/threads;
	printf("Each thread will try to sort %d numbers\n",runlen);
	start=0;
	for (i=0;i<threads;i++) {
		end=start+runlen-1; // because arrays start at 0 end is one less than normal
		// the last thread picks up all the rest
		if ( i==(threads-1) ) {
			end=numbers-1;
		}
		//printf("I will spawn thread id:%d to sort parts from %d to %d\n",i,start,end);
		// create the arguments for the thread
		// each thread has to have its own struct
		thread_args[i].id=i;
		thread_args[i].mode=mode;
		thread_args[i].start=start;
		thread_args[i].end=end;
		// create the thread and pass it some args
		err = pthread_create(&tid[i], NULL,(void *) &worker,(void *)&thread_args[i]);
      	
      	if (err != 0) {
            		printf("Can't create thread :%d",i);
		} else {
            		printf("Thread %d created successfully start=%d end=%d\n",i,thread_args[i].start,thread_args[i].end);
		}

		// for the next iteration
		start=end+1;
	}

	// collect all the threads back to the main program
	// equivalent of wait()
	for (i=0;i<threads;i++) {
		pthread_join( tid[i], NULL);
		printf("Thread %d terminated\n",i);
	}
	// #################################################################################3



	// ##############
	// ############## merging section
	// ##############
	// this is where we have to merge the results from sorting inside the threads
	// we merge a  pair of segments every time
	// this is the actual mergesort algorithm
	int iter=0;
	// each time we double the length of the sort and we half the pairs
	while ( runlen <= numbers ) {
		starta=0;
		iter++;
		printf("Merge sort iteration %d\n",iter);
		// how many pairs do we merge in every iteration
		for (pairs=(numbers/runlen)/2;pairs>=1;pairs--) {
			// calculate start and end of each pair
			enda=starta+runlen-1;
			startb=enda+1;
			endb=startb+runlen-1;
			// last pair gets all the left overs
			if ( pairs == 1 ) {
				endb=numbers-1;
			}	
			printf("I will merge pairs %d-%d and %d-%d \n",starta,enda,startb,endb);
			merge(starta,enda,startb,endb);
			// for the next loop iteration
			starta=endb+1;
		}
		// double the run length , meaning  half the pairs
		runlen=runlen*2;
		// copy the arrays over again for the next iteration
		for (i=0;i<numbers;i++) {
			numbers_ar[i]=merge_ar[i];
		}
	}
	// ######################################################################################
	
	
	// ###############
	// ############### finishing section
	// ###############
	// print the sorted data out
	fprintf(out,"Sorted Array\n");
	fprintf(out,"============\n");
	for (i=0;i<numbers;i++) {
		// print an output line to the file
		fprintf(out,"%d\n",numbers_ar[i]);
	}
	// end, close the file
	fclose(out);
	gettimeofday(&t2,NULL); // find when the program ends
        elapsedTime = (double)(t2.tv_sec - t1.tv_sec) * 1.0e9 +
              (double)(t2.tv_usec - t1.tv_usec)*1000.0;
	printf("Elapsed time %f nanoseconds\n",elapsedTime);
	// ###############################################################################
}


//##################################################
// Parse the command line and fill in the variables
//##################################################
void parse_args(int argc,char **argv) {
	char linebuf[1000];
	char param[200];
	char *tok;
	int value;
	int i;

	// parse the command line
	// we start at 1 because at 0 we have the executing prorgrams name
	for (i=1;i<argc;i++) {
		// zero out the parameter
		param[0]='\0';
		// copy the parameter into linebuf
		strcpy(linebuf,argv[i]);
		// break linebuff apart using = as a delimiter
		tok=strtok(linebuf,"=");
		// this is the parameter
		strcpy(param,tok);
		// if we had an equal this is the value	
		tok=strtok(NULL,"=");
		if ( tok == NULL ) {
			printf("Parameter error, no equal sign?\n");
			// go back to the loop
			continue;
		}
		//ascii to integer
		value=atoi(tok);


		// compare parameter with what is expected
		if (strcmp(param,"-numbers") == 0 ) {
			numbers=value;
		} else if (strcmp(param,"-threads") == 0 ) {
			threads=value;
		} else if (strcmp(param,"-seed") == 0 ) {
			seed=value;
		} else if (strcmp(param,"-mode") == 0 ) {
			mode=value;
		} else {
			printf("Unknown parameter:%s\n",param);
		}
	}
	
	// ##########################################
	// validate the parameters passed
	// ##########################################
	if ( ( threads !=1 ) &&  ( threads !=2 ) &&  ( threads !=4 ) &&  ( threads !=8 ) ) {
		printf("I can only do 1,2,4 or 8 threads\n");
		exit(1);
	}

	if ( ( mode !=1 ) &&  ( mode !=2 ) &&   ( mode !=3 ) ) {
		printf("I can only do modes  1,2 and 3 \n");
		exit(1);
	}
	if ( numbers > NUMBERS ) {
		numbers=NUMBERS;
	}
}

//#########################
// brain dead bubble sort
//#########################
void bubble_sort(int start,int end,int mode) {
	int i,j;
	unsigned int temp=0;

	// printf("Bubble Sorting from %d to %d \n",start,end);

	for (i=start;i<end;i++) {
		for (j=i+1;j<=end;j++) {
			if (numbers_ar[i]>numbers_ar[j]) {
				temp=numbers_ar[j];
				if ( mode==2) { 
					pthread_mutex_lock(&mutex); // exclusive lock on array writing
				}
				numbers_ar[j]=numbers_ar[i];
				numbers_ar[i]=temp;
				if ( mode == 2 ) {
					pthread_mutex_unlock(&mutex); //release exclusive lock on array
				}
			}
		}
	}
}

//###################################################
// merge sort two segments of the numbers_ar
//###################################################
void merge(int starta, int enda, int startb, int endb) {
	int i;
	int j;
	int mi;  // merge array index

/*
	printf("Original Segment\n");
	for (i=starta;i<=endb;i++) {
		printf("offset=%d val=%d\n",i,numbers_ar[i]);
	}
	printf("=================\n\n");
*/
	// i keeps track of the first section
	// j keeps track of the second section
	// mi is the index into the merge section that we are writing currently
	i=starta;
	j=startb;
	mi=starta;
	// move each segment index down and copy over the smallest number
	while ( (i<=enda) && ( j <= endb) && ( mi <= endb) ) {
		// printf("i=%d j=%d mi=%d\n",i,j,mi);
		if (numbers_ar[i]>numbers_ar[j] ) {
			// second section number is smaller
			merge_ar[mi]=numbers_ar[j];
			j++;
            mi++;
		} else if (numbers_ar[i]<numbers_ar[j] ) {
			// first section number is smaller
			merge_ar[mi]=numbers_ar[i];
			i++;
            mi++;
		} else if (numbers_ar[i]==numbers_ar[j] ) { 
			// the numbers are equal copy from both sections
			merge_ar[mi]=numbers_ar[i];
            i++;
			mi++;
            merge_ar[mi]=numbers_ar[j];
            j++;
	        mi++;
		}
	}

	// ################
	// copy left overs
	// ################
	//we are not done yet, 
	// we must copy over what is left out
	//do we have the leftovers from the first segment ?
	while (i<=enda) {
		// printf("spilloveri i=%d j=%d mi=%d\n",i,j,mi);
		merge_ar[mi]=numbers_ar[i];
		mi++;
		i++;
	}

	//do we have the leftovers from the second segment ?
	while (j<=endb) {
		// printf("spilloverj i=%d j=%d mi=%d\n",i,j,mi);
		merge_ar[mi]=numbers_ar[j];
		mi++;
		j++;
	}

}

// ######################################
// this is a working thread
// ######################################
void worker(void *arguments) {
	int start;
	int end;
	int mode;
	int i,id;
	args_for_thread *myargs;	// the arguments structure I got passed

	myargs = arguments;	
	id=myargs->id;
	mode=myargs->mode;
	start=myargs->start;
	end=myargs->end;

	printf("I am thread:%d with start=%d end=%d mode=%d\n",id,start,end,mode);
//	for (i=start;i<=end;i++) {
//		printf("BEFOR Thr:%d,offset=%d,value=%d\n",id,i,numbers_ar[i]); 
//	}

	if ( mode == 1 ) {
		pthread_mutex_lock(&mutex); // exclusive lock on array

		// lock for printf
		pthread_mutex_lock(&printf_mutex);
		printf("I am thread %d and I hold an exclusive lock\n",id);
		// unlock for printf
		pthread_mutex_unlock(&printf_mutex);

		bubble_sort(start,end,mode); 
		
		pthread_mutex_unlock(&mutex);	// release exclusive lock on array

		pthread_mutex_lock(&printf_mutex);
		printf("I am thread %d and I released an exclusive lock\n",id);
		pthread_mutex_unlock(&printf_mutex);

	} else if ( mode == 3 ) {
		pthread_mutex_lock(&printf_mutex);
		printf("I am thread %d and I hold no locks\n",id);
		pthread_mutex_unlock(&printf_mutex);

		bubble_sort(start,end,mode);
	} else if ( mode == 2 ) {
		printf("I am thread %d and I will  lock on writes\n",id);
		bubble_sort(start,end,mode);
	}
	
	// all done print the segment out
	pthread_mutex_lock(&printf_mutex);
	printf("I am thread %d and I am done processing \n",id);
	for (i=start;i<=end;i++) {
		printf("AFTER Thread %d , offset=%d, value=%d\n",id,i,numbers_ar[i]); 
	}
	pthread_mutex_unlock(&printf_mutex);
}
