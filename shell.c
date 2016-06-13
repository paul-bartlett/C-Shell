#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define MAX 256
#define CMD_MAX 10

volatile sig_atomic_t stop;

void inthand(int signum)
{
	stop = 1;
}

// Gets the user name of the terminal
const char *getUserName()
{
	uid_t uid = geteuid();
	struct passwd *pw = getpwuid(uid);
	if(pw)
	{
		return pw->pw_name;
	}

	return "";
}

int make_tokenlist(char *buf, char *tokens[])
{

	char input_line[MAX];
	char *line;
	int i,n;

	i = 0;

	line = buf;
	tokens[i] = strtok(line, " ");
	do {
		i++;
		line = NULL;
		tokens[i] = strtok(line, " ");
	} while(tokens[i] != NULL);

	return i;
}

void mult_io(char *tokens[], int argNum)
{
	int in, out, i = 0;
	char input[MAX], output[MAX], *argv[CMD_MAX];	
	while(strcmp(tokens[i], "<") != 0)
	{
		argv[i] = tokens[i];
		i++;
	}
	argv[i] = NULL;
	strcpy(input, tokens[i+1]); // first token after <
	strcpy(output, tokens[i+3]); // first token after >
	in = open(input, O_RDONLY);	
	if (dup2(in, STDIN_FILENO) < 0)
	{
		perror("Can't duplicate");
		exit(1);
	}
	close(in);
	out = open(output, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if (dup2(out, STDOUT_FILENO) < 0)
	{
		perror("Can't duplicate");
		exit(1);
	}
	close(out);
	execvp(argv[0], argv);
	exit(1);
}

mult_pipe(char *buf, int argNum, int numPipes)
{
	int i = 0, status, n;
	pid_t pid;
	char *cmd[CMD_MAX];

	int pipefds[2*numPipes];
	while(i < numPipes)
	{
		if(pipe(pipefds + i*2) < 0) 
		{
			perror("Couldn't pipe");
			exit(-1);
		}
		i++;
	}

	char input_line[MAX], *argv[MAX];
	char *line;

	i = 0;

	line = buf;
	cmd[i] = strtok(line, "|");
	do {
		i++;
		line = NULL;
		cmd[i] = strtok(line, "|");
	} while(cmd[i] != NULL);

	int cmdNum = i;

	int j = 0; 
	i = 0;

	while(i < cmdNum)
	{
		pid = fork();
		if (pid < 0)
		{
			perror("fork error");
			exit(-1);
		}
		if(pid == 0) 
		{
			if(i+1 < cmdNum)
			{
				if(dup2(pipefds[j+1], 1) < 0)
				{
					perror("Can't duplicate");
					exit(-1);
				}
			}

			if(j != 0)
			{
				if(dup2(pipefds[j-2], 0) < 0)
				{
					perror("Can't duplicate");
					exit(-1);
				}
			}

			int n = 0;

			while (n < 2*numPipes)
			{
				close(pipefds[n]);
				n++;
			}
			make_tokenlist(cmd[i], argv);
			if(execvp(argv[0], argv) < 0) 
			{
				perror("Can't run command");
				exit(-1);
			}
		}
		i++;
		j+=2;
	}
	n = 0;
	while(n < 2*numPipes)
	{
		close(pipefds[n]);
		n++;
	}
	n = 0;
	while (n < numPipes+1) 
	{
		wait(&status);
		n++;
	}
}

int main()
{
	int  argNum;
	char input[MAX], history[MAX], *tokens[CMD_MAX], buf[MAX];
	char userName[20];
	pid_t pid;
	memcpy(userName, getUserName(),20);
	int histCount = 0;
	char cmdHist[10][MAX];
	// signal
	signal(SIGINT, inthand);
	struct sigaction psa;
	psa.sa_handler = inthand;
	sigfillset(&psa.sa_mask);
	psa.sa_flags = 0;
	sigaction(SIGINT, &psa, NULL);
	sigaction(SIGTSTP, &psa, NULL);
	
	while(1) // command line
	{
		argNum = 0;
		printf("%s> ",userName);
		fgets(input, MAX, stdin);
		strtok(input, "\n"); // Remove trailing newline
		strcpy(history, input); // copy for history before destroyed
		strcpy(buf, input); // copy for pipe before destroyed
		if(input != NULL)
			argNum = make_tokenlist(input, tokens); 
		if(strcmp(tokens[0], "exit") == 0)
			exit(1);
		if(strcmp(tokens[0], "history") == 0)
		{
			pid = fork();

			if (pid < 0)
			{
				perror("Problem forking");
				exit(1);
			}
			if(pid > 0) { wait(0); }
			else
			{
				// returns proper number of hist items to show
				int count = (histCount <= 10) ? histCount : 10;
				// if more than 10 items, start off in the middle of the list else beginning
				int start = (histCount > 10) ? histCount % 10 : 0;
				int i = 0;
				for( ; i < count; i++) // prints history list
				{
					printf("%d. %s\n",i+1,cmdHist[(start+i)%10]);
				}
				return 0;
			}
		}
		strcpy(cmdHist[histCount%10], history);
		histCount++; // after history to skip current call being places in list

		char *argv1[10], *argv2[10]; // for seperating commands

		pid = fork();

		if (pid < 0)
		{
			perror("Problem forking");
			exit(1);
		}
		if(pid > 0)
		{
			wait(NULL); // waits until child changes state
			usleep(10000); // waits for 10 millisecond to complete child commands
		}
		else
		{
			int j = 1, m, n, fds[2], numBytes, status, in, out;
			char line[80], input[MAX], output[MAX];

			while(j < argNum)
			{
				if(strcmp(tokens[j], "<") == 0)
				{
					pid = fork();
					if (pid < 0)
					{
						perror("fork error");
						exit(-1);
					}

					if (pid > 0)
					{ // parent	
						wait(NULL);
					}
					else
					{ // child
						n = j+1;
						while(n < argNum) 
						{
							if(strcmp(tokens[n],">") == 0) 
								mult_io(tokens, argNum);
							n++;
						}
						n = 0;
						while(n < j)
						{
							argv2[n] = tokens[n];
							n++;
						}
						argv2[n] = NULL;
						strcpy(input, tokens[j+1]); // first command after <
						in = open(input, O_RDONLY);
						if (dup2(in, STDIN_FILENO) < 0)
						{
							perror("Can't duplicate");
							exit(1);
						}
						close(in);
						status = execvp(argv2[0], argv2); // second command
						if (status < 0)
						{
							perror("parent: exec problem");
							exit(1);
						}
					}
					return 0;
				}
				else if(strcmp(tokens[j], ">") == 0)
				{
					pid = fork();
					if (pid < 0)
					{
						perror("fork error");
						exit(-1);
					}

					if (pid > 0)
					{ // parent
						wait(NULL);							
					}
					else
					{ // child
						m = 0;
						while(m < j)
						{
							argv1[m] = tokens[m];
							m++;
						}
						argv1[m] = NULL;
						strcpy(output, tokens[j+1]);
						out = open(output, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
						if (dup2(out, STDOUT_FILENO) < 0)
						{
							perror("Can't duplicate");
							exit(1);
						}
						close(out);
						status = execvp(argv1[0], argv1); // first command
						if (status < 0)
						{
							perror("parent: exec problem");
							exit(1);
						}
					}
					return 0;
				}
				else if(strcmp(tokens[j], "|") == 0)
				{
					int pipeNum = 1, n = j+1;
					while(n < argNum)
					{
						if(strcmp(tokens[n], "|") == 0)
							pipeNum++;
						n++;
					}

					mult_pipe(buf, argNum, pipeNum);						
					exit(1);
				}
				j++;
			}
			execvp(tokens[0], tokens);
			exit(1);
		}
	}
}
