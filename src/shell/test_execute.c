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
    strcpy(argv[0], "cd");
    argv[1] = (char *) malloc(15);
    strcpy(argv[1], "tests");
    argv[2] = NULL;
    test[0]->argv = argv;
    test[0]->argc = 2;
    printf("Test 1: cd tests\n");
    /* execute_commands(test, 1); */


    /* Redirection test. */
    cmd *test2[1];
    test2[0] = (cmd *) malloc(sizeof(cmd));
    test2[0]->input = (char *) malloc(6);
    test2[0]->output = (char *) malloc(6);
    strcpy(test2[0]->input, "mysh.c");
    strcpy(test2[0]->output, "abc.txt");
    argv = (char **) malloc(2 * sizeof(char *));
    argv[0] = (char *) malloc(3);
    strcpy(argv[0], "cat");
    argv[1] = NULL;
    test2[0]->argv = argv;
    printf("\nTest 2: cat < mysh.c > abc.txt\n");
    execute_commands(test2, 1);

    /* Piping test. */
    cmd *test3[2];
    test3[0] = (cmd *) malloc(sizeof(cmd));
    test3[0]->input = (char *) malloc(6);
    strcpy(test2[0]->input, "mysh.c");
    test3[0]->output = NULL;
    argv = (char **) malloc(2 * sizeof(char *));
    argv[0] = (char *) malloc(3);
    strcpy(argv[0], "cat");
    argv[1] = NULL;
    test3[0]->argv = argv;
    test3[1] = (cmd *) malloc(sizeof(cmd));
    argv = (char **) malloc(3 * sizeof(char *));
    argv[0] = (char *) malloc(4);
    strcpy(argv[0], "grep");
    argv[1] = (char *) malloc(3);
    strcpy(argv[1], "int");
    argv[2] = NULL;
    test[0]->argv = argv;
    printf("\nTest 3: cat < mysh.c | grep int \n");
    execute_commands(test3, 2);

    return 0;
}
