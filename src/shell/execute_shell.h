/*
 * execute_shell.h 
 * Contains utility functions to run the shell, including 
 * those to print the prompt, and execute commands.
 */
#ifndef EXECUTE_SHELL_H
#define EXECUTE_SHELL_H 

#include "shell_structs.h"

void print_prompt();
int execute_commands(cmd **commands, int n);

#endif /* EXECUTE_SHELL_H */
