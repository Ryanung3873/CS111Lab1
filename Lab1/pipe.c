#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
int main(int argc, char *argv[])
{
	if (argc == 1){
		printf("error, not enough parameters");
		return(EINVAL);
	}
	int numPipes = argc - 2;
	if (numPipes <= 0) {
		int return_value = execlp(argv[1], argv[1], NULL);
		printf("Failed");
		return errno;
	}
	else{
		int fd[8][2];
		pipe(fd[0]);
		
		int prevRead = fd[0][0];
		int prevWrite = fd[0][1];
		for (int i = 1; i < argc ; i++){
			pipe(fd[i - 1]);
			pid_t pid = fork();
			if (pid == 0){
				if ( i == 1){
					close(prevRead);
					dup2(prevWrite, 1);
					close(prevWrite);
				}
				// last case
				else if (i == argc - 1){
					close(prevWrite);
					dup2(prevRead, 0);
					close(prevRead);
				}
				else{
					close(prevRead);
					dup2(prevWrite, 1);
					close(prevWrite);
				}
				
				int return_value = execlp(argv[i], argv[i], NULL);
				if (return_value == -1){
					perror("execlp");
				}
				return errno;
			}
			else{
				int status;
				wait(&status);
				if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
					return(WEXITSTATUS(status));
				}
				close(prevRead);
				close(prevWrite);
				prevRead = fd[i - 1][0];
				prevWrite = fd[i - 1][1];
			}
		}

	}
	return 0;
	}

	