#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // error cehck
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command1 command2 ... commandN\n", argv[0]);
        return EINVAL;
    }
    int prev = STDIN_FILENO;
    int pipefd[2];

    // initial pipe
    if (pipe(pipefd) != 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    // iterate thru pipes
    for (int i = 1; i < argc; i++) {
        int pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // child process
        if (pid == 0) {
            if (prev != STDIN_FILENO) {
                dup2(prev, STDIN_FILENO);
                close(prev);
            }
            // if not last, redirect the output to da pipe.
            if (i != argc - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            // executes cmd
            execlp(argv[i], argv[i], NULL);
            perror("execvp");
            exit(EXIT_FAILURE);
        } 
        // parent process
        else {
            int status;
            waitpid(pid, &status, 0);
            int exitStatus = WEXITSTATUS(status);

            if (exitStatus != 0) {
                return exitStatus;
            }

            if (prev != STDIN_FILENO) {
                close(prev);
            }
            prev = pipefd[0];
            close(pipefd[1]);
            pipe(pipefd);
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if (argc >= 2) {
        close(prev);
    } else {
        return 0;
    }

    return 0;
}

