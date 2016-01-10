/*
 * execute_shell.c
 * Contains definitions of shell utility functions. 
 */
#include "execute_shell.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

/* Size of char buffer to hold current directory string. */
#define CWD_BUFFER_SIZE 1000 

/* 
 * Either unistd call might fail, so we encapsulate all 
 * possible outcomes in a status enum.
 */
typedef enum {SUCCESS, LOGIN_FAIL, CWD_FAIL, BOTH_FAIL} error_status;

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

    if (status == SUCCESS) {
        printf("%s:%s> ", login, cwd);
    }
    else {
        printf("Error %d in reading login/cwd. Exiting...\n", status);
        exit(1); /* Exit with error. */ 
    }
}
