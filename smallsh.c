#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "signal.h"
#include "fcntl.h"

//
//global struct to hold all the goodies
//


struct inputarg{
	char instream[2049];
	char * args[512];
	char * infile;
	char * outfile;
	int background;
	int exitstate;
} input;

int allowbackground = 1;

//
//This function gets some input, parses into arguments, then puts it into the global struct
//

void reader(){
	int i;
	int offset = 0;	

//initialize the args array to NULL
	for(i = 0; i < 512; i++){
		input.args[i] = NULL;
	}
//initialize files to NULL
	input.infile = NULL;
	input.outfile = NULL;

//gonna reuse i so I'm putting it back in its place
	i = 0;

//printing a colon and flushing
	printf(":");
	fflush(stdout);

//get input from stdin
	fgets(input.instream, 2048, stdin);

//get the first word from the input	
	input.args[0] = strtok(input.instream," \n");

//get the rest of the words from the input, if there are any
	while(input.args[i] != NULL){
		i++;
		input.args[i] = strtok(NULL," \n");
		if(input.args[i] != NULL) {
			if(strcmp(input.args[i],"<") == 0){
				input.infile = strtok(NULL, " \n");
				input.args[i] = NULL;
				i--;
			} else if(strcmp(input.args[i],">") == 0){
				input.infile = strtok(NULL, " \n");
				input.args[i] = NULL;
				i--;
			}
		}
	}
	if(strcmp(input.args[i-1],"&") == 0) {
		input.background = 1;
		input.args[i-1] = NULL;
	} else {
		input.background = 0;
	}
}

int executor(){

	//these will be reused to hold a couple different results when opening/closing files 
	int result, file;
	
	//forking based on lecure
	pid_t spawnPid = -5;

	spawnPid = fork();

	switch(spawnPid) {
	//error case
		case -1:
			perror("Something went really wrong.\n");
			return 1;
			break; //this break exists to make the compiler happy, but is not necessary
	//child case	
		case 0:
			//input handling
			if(input.infile != NULL){
				//using input to hold return of open
				file = open(input.infile, O_RDONLY);
				if (file == -1) {
					perror("Unable to open input file!\n");
					fflush(stdout);
					return 1;
				}

				//using result to hold return of dup2
				result = dup2(file, 0);
				if (result == -1) {
					perror("Unable to assign input file!\n");
					fflush(stdout);
					return 2;
				}

				//closing the file after assignment
				fcntl(file, F_SETFD, FD_CLOEXEC);
			}
			//output handling
			if(input.outfile != NULL){
				//using input to hold return of open
				file = open(input.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (file == -1) {
					perror("Unable to open output file!\n");
					fflush(stdout);
					return 1;
				}

				//using result to hold return of dup2
				result = dup2(file, 0);
				if (result == -1) {
					perror("Unable to assign output file!\n");
					fflush(stdout);
					return 2;
				}

				//closing the file after assignment
				fcntl(file, F_SETFD, FD_CLOEXEC);
			}

			//actual execution
			if (execvp(input.args[0], input.args)){
				perror("Execution failed!\n");
				fflush(stdout);
				return 2;
			}
			break; //compiler bait
	//parent case
		default:
			if(input.background && allowbackground) {
				pid_t actualPid = waitpid(spawnPid, input.exitstate, WNOHANG);
			} else {
				pid_t actualPid = waitpid(spawnPid, input.exitstate, 0);
			}
			
	}
}


int main () {
	int i = 0;
	int exit = 0;
	char buffer[512];

	getcwd(buffer,512);
//run reader to put the args in the global struct that holds the args
	reader();

//checking for inbuilt commands
	if(strcmp(input.args[0],"exit") == 0){
		return 0; //exits
	} else if(strcmp(input.args[0],"cd") == 0 && input.args[1] == NULL ){
		chdir(getenv("HOME"));
	} else if(strcmp(input.args[0],"cd") == 0 && input.args[1] != NULL ){
		chdir(input.args[1]);
	} else if(strcmp(input.args[0],"status") == 0){
		printf("%d\n", input.exitstate);
		fflush(stdout);
	} else if(strcmp(input.args[0],"#") == 0 || input.args[0] == NULL){
		//do nothing
	} else {
		//if it isn't a built-in then it must be somethign to execute
		executor();
	}


//i'll do it again
	while(main());
	return 0;
}
