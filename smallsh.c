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
	int pid;
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
	if(input.instream != NULL)
		input.args[0] = strtok(input.instream," \n");

//get the rest of the words from the input, if there are any
//
	while(input.args[i] != NULL){
		i++;
		input.args[i] = strtok(NULL," \n");
		if(input.args[i] != NULL) {
			if(strcmp(input.args[i],"<") == 0){
				input.infile = strtok(NULL, " \n");
				printf("file: %s\n",input.infile);
				input.args[i] = NULL;
				i--;
			} else if(strcmp(input.args[i],">") == 0){
				input.outfile = strtok(NULL, " \n");
				printf("file: %s\n",input.outfile);
				input.args[i] = NULL;
				i--;
			} else if(strcmp(input.args[i],"$$") == 0){
				sprintf(input.args[i],"%d",input.pid);
			}
		}
	}

	if(i > 1){
	if(strcmp(input.args[i-1],"&") == 0) {

		input.background = 1;
		input.args[i-1] = NULL;
	} else {
		input.background = 0;
	}
}
}

void statusPrinter() {
	if(WIFEXITED(input.exitstate)) {
		printf("Exited with value: %d\n", WEXITSTATUS(input.exitstate));
	} else {
		printf("Exited with signal: %d\n", WTERMSIG(input.exitstate));
	}
}

int executor(struct sigaction sa){

	//these will be reused to hold a couple different results when opening/closing files 
	int result, infile, outfile;
	
	//forking based on lecure
	pid_t spawnPid = -5;

	spawnPid = fork();

	switch(spawnPid) {
	//error case
		case -1:
			perror("Something went really wrong.\n");
			exit(1);
			break; //this break exists to make the compiler happy, but is not necessary
	//child case	
		case 0:
			//turning the signal back on
			sa.sa_handler = SIG_DFL;
			sigaction(SIGINT, &sa, NULL);

			//input handling
			if(input.infile != NULL){
				//using input to hold return of open
				infile = open(input.infile, O_RDONLY);
				if (infile == -1) {
					perror("Unable to open input file!\n");
					fflush(stdout);
					exit(1);
				}

				//using result to hold return of dup2
				result = dup2(infile, 0);
				if (result == -1) {
					perror("Unable to assign input file!\n");
					fflush(stdout);
					exit(2);
				}

				//closing the file after assignment
				close(infile);
			}
			//output handling
			if(input.outfile != NULL){
				//using input to hold return of open
				outfile = open(input.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (outfile == -1) {
					perror("Unable to open output file!\n");
					fflush(stdout);
					exit(1);
				}

				//using result to hold return of dup2
				result = dup2(outfile, 1);
				if (result == -1) {
					perror("Unable to assign output file!\n");
					fflush(stdout);
					exit(2);
				}

				//closing the file after assignment
				close(outfile);
			}

			//actual execution
			if (execvp(input.args[0], input.args)){
				perror("Execution failed!\n");
				fflush(stdout);
				exit(2);
			}
			break; //compiler bait
	//parent case
		default:
			if(input.background && allowbackground) {
				pid_t actualPid = waitpid(spawnPid, input.exitstate, WNOHANG);
				printf("Background PID: %d\n", spawnPid);
				fflush(stdout);
			} else {
				pid_t actualPid = waitpid(spawnPid, input.exitstate, 0);
			}
		while((spawnPid = waitpid(-1, input.exitstate, WNOHANG)) > 0) {
			printf("child %d killed\n", spawnPid);
			fflush(stdout);
			statusPrinter();
		}
	}
}



void signalCatcher() {
	if (allowbackground == 1) {
		printf("Entering foreground only mode\n");
		fflush(stdout);
		allowbackground = 0;
	} else {
		printf("Leaving foreground only mode\n");
		fflush(stdout);
		allowbackground = 1;
	}
}

int main () {
	int i = 0;
	int loop = 1;
	input.pid =  getpid();

	//signal handling, ingnoring c
	struct sigaction sa_sigint = {0};
	sa_sigint.sa_handler = SIG_IGN;
	sigfillset(&sa_sigint.sa_mask);
	sa_sigint.sa_flags = 0;
	sigaction(SIGINT, &sa_sigint, NULL);

	// Redirectig z to function
	struct sigaction sa_sigtstp = {0};
	sa_sigtstp.sa_handler = signalCatcher;
	sigfillset(&sa_sigtstp.sa_mask);
	sa_sigtstp.sa_flags = 0;
	sigaction(SIGTSTP, &sa_sigtstp, NULL);

while(loop){
	//run reader to put the args in the global struct that holds the args
		reader();
	//checking for inbuilt commands
	if(input.args[0] != NULL)
		if(strcmp(input.args[0],"exit") == 0){
			return 0; //exits
		} else if(strcmp(input.args[0],"cd") == 0 && input.args[1] == NULL ){
			chdir(getenv("HOME"));
		} else if(strcmp(input.args[0],"cd") == 0 && input.args[1] != NULL ){
			chdir(input.args[1]);
		} else if(strcmp(input.args[0],"status") == 0){
			printf("%d\n", input.exitstate);
			fflush(stdout);
		} else if(*(input.args[0]) == '#'){
			//do nothing
		} else {
			//if it isn't a built-in then it must be somethign to execute
			executor(sa_sigint);
		}
}
return 0;
}
