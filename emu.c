/*
*	Seniha Sena Topkaya
*	131044020
*	Hw2
*
*/

/*added for std=c11 if makefile will be changed*/
#define _POSIX_C_SOURCE 200809L

/* libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h> 
#include <errno.h>   
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

#define SIZE 1024

/*function declerations */
void commandDetect(char command[SIZE]);
void commandExe(char command[SIZE]);
void inpRedirect(char command[SIZE], char file[SIZE]);
void outRedirect(char command[SIZE], char file[SIZE]);
void myPipe(char command[SIZE], char command2[SIZE]);
void signalHandler(int sig);
void killHandler(int sig);
void logFile(int fd, pid_t pid, char command[SIZE]);

struct sigaction sa; 	/* struct for handling signals */
pid_t allPids[SIZE];	/* pids of all processes*/
int idx=0;
int fd; 				/*log file desc*/
char filename[50];

int main(int argc, char *argv[])
	{
	char allCommands[SIZE][SIZE]; 	/* identifiers */
	char command[SIZE];
	char inp[SIZE];
	char cwd[SIZE];
    time_t t;

	if (argc!=1 || strcmp(argv[0],"./emu")!=0) /* usage printing */
		{
		fprintf(stderr, "Usage: ./emu\n");
		exit(EXIT_FAILURE);
		}

	/*____________________________________________signal mask*/

	sa.sa_handler = signalHandler; 
   	sa.sa_flags = SA_RESTART;
    
	if (sigemptyset(&sa.sa_mask) == -1)
		{
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
		}
	/* SIGINT signals are added */
	if(sigaddset(&sa.sa_mask, SIGINT) == -1)
		{
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
		}
	/* SIGTERM signals are added */
	if(sigaddset(&sa.sa_mask, SIGTERM) == -1)
		{
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
		}
	/* SIGSTOP signals are added */
	if(sigaddset(&sa.sa_mask, SIGTSTP) == -1)
		{
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
		}
	/*____________________________________________signal mask*/

	fprintf(stderr, "\n______________________________\n");
	fprintf(stderr, "\n      My Shell Emulator       \n");
	fprintf(stderr, "\n is waiting for your command..  ");
	fprintf(stderr, "\n______________________________\n");

	for (;;)
		{
		/*____________________________________________signal handler*/
		/* Signal Handler is called if SIGINT catched */
		if(sigaction(SIGINT, &sa, NULL) == -1)
			{
			perror("\tCan't SIGINT");
			}
		/* Signal Handler is called if SIGTERM catched */
		if(sigaction(SIGTERM, &sa, NULL) == -1)
			{
			perror("\tCan't SIGTERM");
			}
		/* Signal Handler is called if SIGTERM catched */
		if(sigaction(SIGTSTP, &sa, NULL) == -1)
			{
			perror("\tCan't SIGTSTP");
			}
		/* Signal Handler is called if SIGINT catched */
		/*____________________________________________signal handler*/

		fprintf(stdout, "\n \n\n\033[0;32m emu@myshellemu:\033[0;35m~%s$\033[0;37m\n >>>", getcwd(cwd, sizeof(cwd)));
		fgets(inp, SIZE, stdin); 	/* ______getting shell command */

		/*push null end of the string*/
		if (inp[strlen(inp)-1]=='\n')
			{ 
			inp[strlen(inp)-1]='\0'; 
			}
		/*exit program - quit command*/
		if (strcmp(inp, ":q")==0)
			{ 
			exit(EXIT_SUCCESS);
			}
		/*exit program - quit command*/
		if (strcmp(inp, "kill -9")==0)
			{ 
			killHandler(SIGKILL);
			}

		/*____________________________________________log file part*/
		/* log file part */
		for (int i = 0; i < strlen(filename); ++i)
			{
			filename[i]='\0'; 		/*reset the string*/
			}
	   	time(&t);
	    sprintf(filename, "%s.log", ctime(&t));
	    int fd=open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
	    if (fd ==-1) 
	    	{
	        perror("log open:");
	    	}
	    if (close(fd)==-1)
	    	{
	    	perror("close log:");
	    	}
		/*____________________________________________log file part*/

		/*check if there are multiple commands*/
		int is_operator=0;
		for (int i = 0; i < strlen(inp); ++i)
			{
			if (inp[i]=='|' || inp[i]=='<' || inp[i]=='>')
				{
				is_operator=1;
				}
			}
		/*ls, pwd, df etc.*/
		if (is_operator==0)	/* __________________________one command */
			{ 
			commandExe(inp);
			}
		else 				/* ____________________multiple commands */
			{
			int cmndCount=0;
			char*token=strtok(inp,"|");		/* input split into pipe */
			while(token!=NULL)
				{
				sprintf(command, "%s", token);
				sprintf(allCommands[cmndCount], "%s", command); /* getting all commands */
				cmndCount++;									/* number of commands */
				token=strtok(NULL,"|");
				}
			/*for (int i = 0; i < cmndCount; i++){ fprintf(stderr, "-%s \n", allCommands[i]); }*/	
			
			/*too many commands-quit*/
			if (cmndCount==20 || cmndCount>20)
				{
				fprintf(stderr, "Too many shell command entered!!\n");
				exit(EXIT_FAILURE);
				}
			/*command included redirection or pipe */
			if (cmndCount==1 && is_operator==1)
				{ 
				/*fprintf(stderr, "[%s]\n", inp);*/
				commandDetect(inp);					/*detection executes*/
				}
			else if (cmndCount>1 && cmndCount%2==0) /*multiple commands, even number*/
				{
				for (int i = 0; i < cmndCount; i+=2)
					{
					/*fprintf(stderr, "even num command: [%s] - [%s]\n", allCommands[i], allCommands[i+1]);*/
					myPipe(allCommands[i], allCommands[i+1]);
					}
				}
			else if (cmndCount>1 && cmndCount%2!=0) /*multiple commands, odd number*/
				{
				for (int i = 0; i < cmndCount-1; i+=2)
					{ 
					/*fprintf(stderr, "odd num command: [%s] - [%s]\n", allCommands[i], allCommands[i+1]);*/
					myPipe(allCommands[i], allCommands[i+1]);
					}
				/*fprintf(stderr, "odd num last command: [%s]\n", allCommands[cmndCount-1]);*/
				commandDetect(allCommands[cmndCount-1]);
				}
			}
		/*____________________________________________signal handler*/
		/* Signal Handler is called if SIGINT catched */
		if(sigaction(SIGINT, &sa, NULL) == -1)
			{
			perror("\tCan't SIGINT");
			}
		/* Signal Handler is called if SIGTERM catched */
		if(sigaction(SIGTERM, &sa, NULL) == -1)
			{
			perror("\tCan't SIGTERM");
			}
		/* Signal Handler is called if SIGTERM catched */
		if(sigaction(SIGTSTP, &sa, NULL) == -1)
			{
			perror("\tCan't SIGTSTP");
			}
		/*____________________________________________signal handler*/
		}

	return 0;
	}


/* this function detects the command and calls the corresponding function*/
void commandDetect(char command[SIZE])
	{
	/*	fprintf(stderr, "commandDetect:[%s]\n", command);*/
	int status=0;
	for(int i=0; i<strlen(command); i++)
		{
		if(command[i]=='>') /*output redirection needed*/
			{
			status=1;
			}
		if(command[i]=='<') /*input redirection needed*/
			{
			status=2;
			}
		}
	if (status==0)
		{
		commandExe(command); /*single command executed*/
		}
	else if (status==1) 	/*__________output redirection__________*/
		{
		int j=0;
		char commandTemp[SIZE][SIZE];					/*identifiers to split*/
		char commandFirst[SIZE], commandLast[SIZE];

		char*token=strtok(command,">");
		while (token!=NULL)
			{ 											/*split into >*/
			sprintf(commandTemp[j], "%s", token); 		/* store commands in array */
			j++;
			token=strtok(NULL,">");
			}
		/*for (int i = 0; i < j; ++i){ fprintf(stderr, "%s\n", commandTemp[i]); }*/

		char*token2=strtok(commandTemp[1]," ");
		while (token2!=NULL)
			{
			sprintf(commandLast, "%s", token2); 		/* gettin file arg */
			token2=strtok(NULL," ");
			}

		sprintf(commandFirst, "%s", commandTemp[0]); 	/*gettin command*/
		outRedirect(commandFirst,commandLast); 			/* proper func is called*/
		}
	else if(status==2)  	/*__________input redirection__________*/
		{
		int j=0;
		char commandTemp[SIZE][SIZE];					/*identifiers to split*/
		char commandFirst[SIZE], commandLast[SIZE];

		char*token=strtok(command,"<");
		while (token!=NULL)
			{											/*split into <*/
			sprintf(commandTemp[j], "%s", token); 		/* store commands in array */
			j++;
			token=strtok(NULL,"<");
			}
		/*for (int i = 0; i < j; ++i){ fprintf(stderr, "%s\n", commandTemp[i]); }*/

		char*token2=strtok(commandTemp[1]," ");
		while (token2!=NULL)
			{
			sprintf(commandLast, "%s", token2); 		/* gettin file arg*/
			token2=strtok(NULL," ");
			}
		sprintf(commandFirst, "%s", commandTemp[0]); 	/*gettin command*/
		inpRedirect(commandFirst,commandLast);		 	/* proper func is called*/
		}
	else
		{
		fprintf(stderr, "\nError:Invalid shell command\n");
		}
	}

/*It is used to handle any shell command*/
void commandExe(char command[SIZE])
	{
	/*fprintf(stderr, "commandExe:[%s]\n", command);*/
	int status;

	pid_t pid=fork();		/* create a new process */
	if (pid<0)
		{
		perror("fork:");
		}
	if (pid==0) 			/* child process handle the given command */
		{
		allPids[idx++]=pid;					/*pid is stored*/
		logFile(fd, getpid(), command);		/*log file func call*/

		if (execl("/bin/sh", "sh", "-c", command, (char *) NULL)==-1) /*command done*/
			{
			perror("execl:");
			}
		_exit(EXIT_SUCCESS);
		}
	else
		{
		allPids[idx++]=pid;			/*pid is stored*/
		if (waitpid(pid, &status, 0)==-1) 	/* parent is waiting */
			{
			perror("close waitpid:");
			}
		}
	}

/* sets standard output as given file: > */
void outRedirect(char command[SIZE], char file[SIZE])
	{
	/*fprintf(stderr, "outRedirect:[%s][%s]\n", command, file);*/
	int status;

	pid_t pid=fork();		/* create a new process */
	if (pid<0)
		{
		perror("fork:");
		}
	if (pid==0) 			/* child process handle the command */
		{
		allPids[idx++]=pid;					/*pid is stored*/
		/* create an output file to write */
		int fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0644); 
		if (fd == -1)
			{
			perror("open");
			}

		if (dup2(fd, STDOUT_FILENO)==-1) 	/* output desc is duplicated */
			{
			perror("dup2:");
			}

		if (close(fd)==-1)					/* close file desc */
			{
			perror("close:");
			}

		logFile(fd, getpid(), command);		/*log file func call*/

		if(execl("/bin/sh", "sh", "-c", command, (char *) NULL)==-1) /*command done*/
			{
			perror("execl:");
			}
		_exit(EXIT_SUCCESS);
		}
	else
		{
		allPids[idx++]=pid;					/*pid is stored*/
		if (waitpid(pid, &status, 0)==-1)  	/* parent is waiting */
			{
			perror("close waitpid:");
			}
		}
	}

/* sets standard input as given file: <*/
void inpRedirect(char command[SIZE], char file[SIZE])
	{
	/*fprintf(stderr, "inpRedirect:[%s][%s]\n", command, file);*/
	int status;

	pid_t pid=fork(); 			/* create a new process */
	if (pid<0)
		{
		perror("fork:");
		}
	if (pid==0) 				/* child process handle the command */
		{
		allPids[idx++]=pid;				/*pid is stored*/
		int fd=open(file, O_RDONLY); 	/* open to read the inp file */
		if (fd==-1)
			{
			perror("open:");
			}

		if (dup2(fd, STDIN_FILENO)==-1) /* input desc is duplicated */
			{
			perror("dup2:");
			}

		if (close(fd)==-1) 				/* close file */
			{
			perror("close:");
			}

		logFile(fd, getpid(), command); /*log file func call*/

		if(execl("/bin/sh", "sh", "-c", command, (char *) NULL)==-1) /*command done*/
			{
			perror("execl:");
			}
		_exit(EXIT_SUCCESS);
		}
	else
		{
		allPids[idx++]=pid;					/*pid is stored*/
		if (waitpid(pid, &status, 0)==-1)  	/* parent is waiting */
			{
			perror("close waitpid:");
			}
		}		
	}

void myPipe(char command[SIZE], char command2[SIZE])
	{
	/*fprintf(stderr, "pipe:[%s][%s]\n", command, command2);*/
	int pfd[2];
	if (pipe(pfd)==-1)	/* pipe created */
		{
		perror("pipe:");
		}

	pid_t pid1=fork();			/* new process created */
	if (pid1 < 0)
		{
		perror("fork:");
		}
	else if (pid1 == 0) 		/* first child for command before pipe x| */
		{
		allPids[idx++]=pid1;					/*pid is stored*/
		logFile(fd, getpid(), command);			/*log file func call*/

		if (dup2(pfd[1], STDOUT_FILENO)==-1)	/* write-end is duplicated */
			{
			perror("dup2:");
			}

		commandDetect(command);		/*called my func to detect the command*/

		if (close(pfd[0])==-1)		/* read-end is closed */
			{
			perror("close pipe:");
			}
		if (close(pfd[1])==-1) 		/* write-end is closed */
			{
			perror("close pipe:");
			}
		_exit(EXIT_SUCCESS);
		}

	pid_t pid2 = fork();	/* new process created */
	if (pid2 < 0)
		{
		perror("fork:");
		}
	else if (pid2 == 0) 	/* second child for command after pipe |x */
		{
		allPids[idx++]=pid2;					/*pid is stored*/
		logFile(fd, getpid(), command2); 		/*log file func call*/
		
		if (dup2(pfd[0], STDIN_FILENO)==-1) 	/* read-end is duplicated */
			{
			perror("dup2:");
			}
		
		if (close(pfd[0])==-1)		/* read-end is closed */
			{
			perror("pipe close:");
			}
		if (close(pfd[1])==-1) 		/* write-end is closed */
			{
			perror("pipe close:");
			}

		commandDetect(command2);	/*called my func to detect the command*/		

		_exit(EXIT_SUCCESS);
		}
	allPids[idx++]=pid1;			/*pid is stored*/
	allPids[idx++]=pid2;			/*pid is stored*/

	if (close(pfd[0])==-1)			/* read-end is closed */
		{
		perror("pipe close:");
		}
	if (close(pfd[1])==-1)			/* write-end is closed */
		{
		perror("pipe close:");
		}
	if (waitpid(pid1, NULL, 0)==-1) 	/* parent is waiting */
		{
		perror("pipe waitpid:");
		}
	if (waitpid(pid2, NULL, 0)==-1) 	/* parent is waiting */
		{
		perror("pipe waitpid:");
		}
	}


/*kill all process to avoid zombies*/
void killHandler(int sig)
	{
	int status;
	fprintf(stderr, "\nSIGKILL signal catched!\n");
	for (int i = 0; i < idx; ++i)
		{
		if(waitpid(allPids[i], &status, 0)==-1)	/*wait for all process termination*/
			{
			perror("waitpid:");
			}
		}	
	for (int i = 0; i < idx; ++i)
		{
		kill(allPids[i], sig);	/*all process are killed*/
		}
	if (close(fd)==-1)			/*log file is closed*/
		{
		perror("close:");
		}
	exit(EXIT_SUCCESS);
	}

/* signal handler function */
void signalHandler(int sig)
	{
	if(sig == SIGINT)
		{
		perror("SIGINT");
		fprintf(stderr, "\n\nSIGINT signal catched!\n");
		}
	else if(sig == SIGTERM)
		{
		perror("SIGTERM");
		fprintf(stderr, "\nSIGTERM signal catched!\n");
		}
	else if(sig == SIGTSTP)
		{
		perror("SIGTSTP");
		fprintf(stderr, "\nSIGTSTP signal catched!\n");
		}
	else if(sig == SIGKILL)
		{
		perror("SIGKILL");
		fprintf(stderr, "\nSIGKILL signal catched!\n");
		}
	}

/*log file keeps the information about processes*/
void logFile(int fd, pid_t pid, char command[SIZE])
	{
	char str[SIZE];
	fd=open(filename, O_WRONLY | O_APPEND);
    if (fd==-1)
    	{
    	perror("open:");
    	}
    sprintf(str, "Process id:%d & Its command: %s\n", pid, command);
    if (write(fd, str, strlen(str))==-1)
    	{
    	perror("log write:");
    	}
    if (close(fd)==-1)
    	{	
    	perror("close:");
    	}
	}

