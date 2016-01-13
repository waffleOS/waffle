/**
 * execute_shell.c
 * Contains definitions of shell utility functions. 
 */
#include "execute_shell.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* Size of char buffer to hold current directory string. */
#define CWD_BUFFER_SIZE 1000

/** 
 * Either unistd.h call might fail, so we encapsulate all 
 * possible outcomes in a status enum.
 */
typedef enum {SUCCESS, LOGIN_FAIL, CWD_FAIL, BOTH_FAIL} error_status;


/**
 * void print_prompt()
 * Prints the shell prompt in the form "usr:cwd> " using the unistd.h API.
 * Exits if there is an error in getlogin() or getcwd(). This may be due
 * to buffer overflow, or memory issues.
 */
void print_prompt() {
    char *login = getlogin();
    char buf[CWD_BUFFER_SIZE];
    char *cwd = getcwd(buf, CWD_BUFFER_SIZE);
    
    error_status status = SUCCESS; 

    if (login == NULL) {
        status = LOGIN_FAIL;
    }

    if (cwd == NULL && status == LOGIN_FAIL) {
        status = BOTH_FAIL;
    }
    else if (cwd == NULL) { 
        status = CWD_FAIL;
    }

    /* Print prompt even if there is an error for debugging purposes. */
    printf("%s:%s> ", login, cwd);
    
    /* If there is an error, alert the user and terminate. */
    if (status != SUCCESS) {
        printf("\nError %d in reading login or cwd. Exiting...\n"
               "Error 1: LOGIN_FAIL, Error 2: CWD_FAIL, Error 3: "
               "BOTH_FAIL\n", status);
        exit(1); /* Exit with error. */ 
    }
}

/**
 * int execute_commands(cmd **commands, int n)
 * Executes a sequence of commands, piping between 
 */
int execute_commands(cmd **commands, int n) {
    int i;
    int *fd = malloc(4 * n * sizeof(int));
    int status;
    pid_t pid;
    
    /* Create pipes. */
    for (i = 0; i < 4 * n; i += 2) {
        /* 
         * fd + 2 * k  corresponds to the pipe for
         * the parent process to write to the k-th process
         * and fd + 2 * k + 2 corresponds to the pipe for
         * the parent process to read from the k-th process. */
        if (pipe(fd + i) < 0) {
            perror("Pipe error");
            exit(EXIT_FAILURE);
        }
    }
    
    for (i = 0; i < n; i++) {
        char **argv = commands[i]->argv;
        int argc = commands[i]->argc;

        /* 
         * Functions that change program state must be implemented
         * before forking.
         */
        if (strcmp(argv[0], "cd") == 0) {
            char *dir;
            char home_dir[CWD_BUFFER_SIZE];
            if (argc == 1) { 
                strcpy(home_dir, "/home/");
                char *login = getlogin();
                strncat(home_dir, login, CWD_BUFFER_SIZE);
                dir = home_dir;
            }
            else if (argc > 1) { 
                dir = argv[1];  
                printf("%s\n", dir);
            }

            if (dir == NULL || chdir(dir) == -1) {
                perror("Invalid directory.");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argv[0], "exit") == 0) { 
            exit(EXIT_SUCCESS);
        } 
        else if ((pid = fork()) < 0) { 
            perror("Fork error");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0) {
            close(fd[4 * i]);
            if (dup2(fd[4 * i + 1], fd[0]) != fd[0]) {
                perror("dup2 error to stdout");
                exit(EXIT_FAILURE);
            }

            wait(&status);
        }
        else { /* child process */
            int in_fd, out_fd;
            
            char *input = commands[i]->input;
            char *output = commands[i]->output;
            
            if (input != NULL) {
                in_fd = open(input, O_RDONLY);
                if (dup2(in_fd, STDIN_FILENO) != STDIN_FILENO) {
                    perror("dup2 error to stdin");
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }
            else if (i != 0) {
                close(fd[2 * i + 1]);
                if (dup2(fd[2 * i], STDIN_FILENO) != STDIN_FILENO) {
                    perror("dup2 error to stdin"); 
                    exit(EXIT_FAILURE);
                }
            }

            if (output != NULL) {
                out_fd = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0);
                if (dup2(out_fd, STDOUT_FILENO) != STDOUT_FILENO) {
                    perror("dup2 error to stdout");
                    exit(EXIT_FAILURE);
                }
                close(out_fd);
            }
            else if (i != n - 1) {
                close(fd[2 * i]);
                if (dup2(fd[2 * i + 1], STDOUT_FILENO) != STDOUT_FILENO) {
                    perror("dup2 error to stdout");
                    exit(EXIT_FAILURE);
                }
            }

            execvp(argv[0], argv);
        }
    }

    /* Clean up pipes and file descriptor memory. */
    for (i = 0; i < 2 * n; i++) {
        close(fd[i]);
    }
    free(fd);

    return 0;
}
