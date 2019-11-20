#include "string.h"
#include "stdio.h"
#include "unistd.h"

//
//global struct to hold all the goodies
//


struct inputarg{
	char instream[2049];
	char * args[512];
} input;

//
//This function gets some input, parses into arguments, then puts it into the global struct
//

void reader(){
	int i;

//initialize the args array to NULL
	for(i = 0; i < 512; i++){
		input.args[i] = NULL;
	}

//gonna reuse i so I'm putting it back in its place
	i = 0;

//printing a colon and flushing
	printf(":");
	fflush(stdout);

//get input from stdin
	fgets(input.instream, 2048, stdin);

//get the first word from the input	
	input.args[i] = strtok(input.instream," \n");

//get the rest of the words from the input, if there are any
	while(input.args[i] != NULL){
		i++;
		input.args[i] = strtok(NULL," \n");
	}
}


int main () {
	int i = 0;
	int exit = 0;

//run reader to put the args in the global struct that holds the args
	reader();

//checking for inbuilt commands
	if(strcmp(input.args[0],"exit") == 0){
		return 0; //exits
	} else if(strcmp(input.args[0],"cd") == 0 && input.args[1] == NULL ){
		printf("changing to %s\n", getenv("HOME"));
		chdir(getenv("HOME"));
	}
	


//i'll do it again
	while(main());
	return exit;
}
