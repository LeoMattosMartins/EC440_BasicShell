#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
 
#define MAX_BUFFER_SIZE 512
#define MAX_ARGS_SIZE 32

#define IN 0
#define OUT 1

void sigcatcher() { /* signal already catches the exit code of the child, so sigcatcher doesn't need to do anything  */ }

void ohno(char* failed)
{
	/* handle all perror()s and exit(2)s to make main easier to read */
	
	if( failed == "open" )		{ perror("ERROR: open() failed to execute"); }

	if( failed == "dup2" ) 		{ perror("ERROR: dup2() failed to execute"); }	

	if( failed == "close" ) 	{ perror("ERROR: close() failed to execute"); }

	if( failed == "pipe" ) 		{ perror("ERROR: pipe() failed to execute"); }
	
	if( failed == "fork" ) 		{ perror("ERROR: fork() failed to execute"); }

	if( failed == "execvp" ) 	{ perror("ERROR: execvp() failed to execute"); }

	exit(2);
}

void redirect(char* file, int type)
{
	int fd;

	/* user has read and write permission */
	fd = ( type )  ? ( open(file, O_WRONLY | O_CREAT, (S_IRUSR + S_IWUSR)) ) : ( open(file, O_RDONLY) );
	
	if( fd == -1 ) { ohno("open"); }

	if( dup2(fd, type) == -1 ) { ohno("dup2"); }
	
	if( close(fd) == -1 ) { ohno("close"); }
}	

int main(int argc, char** argv)
{
	/* declarations */
	char buffer[MAX_BUFFER_SIZE];
	
	char* inputRedirected;
	char* outputRedirected;
	char* backgroundProcess;
	char* filename;
	char* tolkien;
	char* tolkien2;

	char* commandPrompt = "my_shell$ ";
	char* delimiter = "|\n";
	char* delimiter2 = " <>&";
	char* silentFlag = "-n";
	
	char* tolkienArray[MAX_ARGS_SIZE];
	char* tolkienArray2[MAX_ARGS_SIZE];	

	int ii; /* general purpose iterator */
	int jj; /* 2nd layer iterator */
	int isSilent;
	int tolkienArraySize;
	int tolkienArraySize2;

	int statuses[MAX_ARGS_SIZE];
	int pipes[(MAX_ARGS_SIZE + 1)][2];
	
	pid_t pids[MAX_ARGS_SIZE];
		
	/* check for silent (-n) flag */
	isSilent = ( argc > 1 ) ? ( !strcmp(argv[1], silentFlag) ) : ( 0 );

	signal (SIGCHLD, sigcatcher);

	while( 1 )
	{
		if( !isSilent ) { printf("%s", commandPrompt); }

		if( !fgets(buffer, sizeof(buffer), stdin) ) { break; }

		if( !strcmp(buffer, strchr(buffer, '\n')) ) { continue; }

		inputRedirected = strchr(buffer, '<');
		outputRedirected = strchr(buffer, '>');
		backgroundProcess = strchr(buffer, '&');
	
		/* first token of buffer */
		for( ii = 0; ii < MAX_ARGS_SIZE; ii++ ){ tolkienArray[ii] = NULL; }
		tolkien = strtok(buffer, delimiter);
		tolkienArraySize = 0;

		/* pipe tokenisation */
		while( tolkien )
		{
			tolkienArray[tolkienArraySize] = tolkien;
			tolkien = strtok(NULL, delimiter);
			tolkienArraySize++;		
		}

		/* pipe initialisation */ 
		for( ii = 0; ii < (tolkienArraySize - 1); ii++ ) { if( pipe(pipes[ii]) == -1 ){ ohno("pipe"); } }

		for( ii = 0; ii < tolkienArraySize; ii++ )
		{	
			/* reset 2 token array and start 2nd tokenisation */
			for( jj = 0; jj < MAX_ARGS_SIZE; jj++ ){ tolkienArray2[jj] = NULL; }
			tolkien2 = strtok(tolkienArray[ii], delimiter2);
			tolkienArraySize2 = 0;
	
			while( tolkien2 )
			{
				tolkienArray2[tolkienArraySize2] = tolkien2;
				tolkien2 = strtok(NULL, delimiter2);
				tolkienArraySize2++;		
			}

			tolkienArray2[tolkienArraySize2] = NULL; 

			/* child process creation */

			pids[ii] = fork();
			
			if( pids[ii] == -1 ){ ohno("fork"); }
		
			else if( pids[ii] == 0 )
			{
                                /* oldest child — input redirection */
				if(ii == 0)
				{
                                        if( inputRedirected ) { redirect(strtok(inputRedirected, delimiter2), IN); }
                                        
					if( tolkienArraySize > 1 )
                                        {
						/* write output to pipe */
                                        	if( close(pipes[0][IN]) == -1 ) { ohno("close"); }
						if( dup2(pipes[0][OUT], OUT) == -1 ) { ohno("dup2"); }
						if( close(pipes[0][OUT]) == -1 ) { ohno("close"); }
					}
                                        else
                                        {
                                                /* if there's only one token => no pipes required */
                                                if( outputRedirected )
                                                {
                                                        /* remove 2nd to last element - the output file name */
							redirect(strtok(outputRedirected, delimiter2), OUT);
                                                        tolkienArray2[tolkienArraySize2 - 1] = NULL;
                                                }
                                        }

                                        if( execvp(tolkienArray2[0], tolkienArray2) == -1 ) { ohno("execvp"); }
                                }
                                /* youngest child — output redirection */
                                if( ii == (tolkienArraySize - 1) )
                                {
                                        if( outputRedirected ) 
					{ 
						/* remove 2nd to last element - the output file name */
						redirect(strtok(outputRedirected, delimiter2), OUT); 
						tolkienArray2[tolkienArraySize2 - 1] = '\0'; 
					}	

                                        if( tolkienArraySize > 1 )
                                        {
						/* read input from pipe */
						if( dup2(pipes[ii - 1][IN], IN) == -1 ) { ohno("dup2"); }
						if( close(pipes[ii - 1][IN]) == -1 ) { ohno("close"); }
						if( close(pipes[ii - 1][OUT]) == -1 ) { ohno("dup2"); }
					}

                                        tolkienArray2[tolkienArraySize2] = '\0';

                                        if( execvp(tolkienArray2[0], tolkienArray2) == -1 ) { ohno("execvp"); }
                                }

                                /* middle children — piping */
                                else 
				{
					/* write output to pipe */
					if( close(pipes[ii - 1][OUT]) == -1) { ohno("close"); }	
					if( dup2(pipes[ii - 1][IN], IN) == -1) { ohno("close"); }
					if( close(pipes[ii - 1][IN]) == -1) { ohno("close"); }
				
					/* read input from pipe */
					if( close(pipes[ii][IN]) == -1) { ohno("close"); }
					if( dup2(pipes[ii][OUT], OUT) == -1) { ohno("close"); }
					if( close(pipes[ii][OUT]) == -1) { ohno("close"); }

					tolkienArray2[tolkienArraySize2] = '\0';

                                        if( execvp(tolkienArray2[0], tolkienArray2) == -1 ) { ohno("execvp"); }
                                }
			}
			else
			{
				if( ii > 0 )
				{
					close(pipes[ii - 1][OUT]);
					close(pipes[ii - 1][IN]);
				}				
				
				if( !backgroundProcess ) { waitpid(pids[ii], NULL, 0); }
			}
			/* resetting for the next iteration */
			for( jj = 0; jj < tolkienArraySize2; jj++ ) { tolkienArray2[jj] = '\0'; }
		}
		for( ii = 0; ii < tolkienArraySize; ii++ ) { tolkienArray[ii] = '\0'; }
		fflush(NULL);
	}
	printf("\n");
	return 0;
}
