/**
 * test_execute.c
 * Unit testing for execute_commands().
 */
#include "execute_shell.h"
#include "shell_structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    /* Simple command run. */
    cmd *test[1];
    test[0] = (cmd *) malloc(sizeof(cmd));
    char **argv = (char **) malloc(3 * sizeof(char *));
    argv[0] = (char *) malloc(3);
    strcpy(argv[0], "cat");
    argv[1] = (char *) malloc(15);
    strcpy(argv[1], "shell_structs.h");
    argv[2] = NULL;
    test[0]->argv = argv;
    printf("Test 1: cat shell_structs.h\n");
    execute_commands(test, 1);

    /* Redirection test. */
    cmd *test2[1];
    test2[0] = (cmd *) malloc(sizeof(cmd));
    test2[0]->input = (char *) malloc(6);
    test2[0]->output = NULL;
    strcpy(test2[0]->input, "mysh.c");
    argv = (char **) malloc(3 * sizeof(char *));
    argv[0] = (char *) malloc(2);
    strcpy(argv[0], "cat");
    argv[1] = NULL;
    printf("\nTest 2: cat < mysh.c\n");
    execute_commands(test2, 1);

    return 0;
}
