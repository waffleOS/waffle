/*******************************************************************************
 * parser_tests.c
 * CS124 Team Waffle Assignment 1 Shell
 * parser_tests.c
 *
 * This file includes functions to test the tokenize and parsing functions.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parse.h"

/* Print out tokens in array. */
void print_tokens(token **tokens, int n) {
    int i;
    for(i = 0; i < n; i++) {
        printf("Token %d of type %d: %s\n", i, tokens[i]->type, tokens[i]->text);
    }
}

/* Print out commands in array. */
void print_commands(cmd **commands, int n) {
    int i, j;
    for(i = 0; i < n; i++) {
        printf("Command %d with %d arguments. Arguments:\n", i, commands[i]->argc);
        for(j = 0; j < commands[i]->argc; j++) {
            printf("%s\n", (commands[i]->argv)[j]);
        }

        if(commands[i]->input == NULL) {
            printf("Input is stdin.\n");
        } else {
            printf("Input is %s.\n", commands[i]->input);
        }

        if(commands[i]->output == NULL) {
            printf("Output is stdout.\n");
        } else {
            printf("Output is %s.\n", commands[i]->output);
        }

        if(commands[i]->error == NULL) {
            printf("Error is stderr.\n");
        } else {
            printf("Error is %s.\n", commands[i]->error);
        }
    }

}


/*
 * Test tokenize function and returns status of test.
 * token **tokenize(char *buf, int *num_tokens);
 */
int test_tokenize(char *test_str) {
    token **tkns;
    int expected_num_tkns;
    // printf("%d tokens for test string:\n", expected_num_tkns);
    printf("%s\n", test_str);
    initializeParser();
    tkns = tokenize(test_str, &expected_num_tkns);
    printf("Tokenize called. Returned:\n");
    print_tokens(tkns, expected_num_tkns);
    test_command(tkns, expected_num_tkns);
    /* return strcmp();*/
    return 0;
}

int test_command(token * tokens, int num_tokens)
{
    int num_commands;
    cmd ** cmds;
    cmds = parse(tokens, num_tokens, &num_commands);
    printf("Parse called. Returned:\n");
    print_commands(cmds, num_commands);
    return 0;
}

/*
 * Unit test parse function
 * Tests the parse function given a specified
 * array of tokens and returns the status of the test.
 */
/*int unit_test_parse(token **tokens);
*/
/* Integration test of tokenize and parse. */
/*int int_test_parse(char *test_str, int expected_num_tkns) {
*/    /* Sample implementation */
/*    int token_status = test_tokenize(test_str, expected_num_tkns);
*//*    if (token_status == TOKEN_GOOD) {
*/        /* TODO: set up tokens using tokenize and a buffer. */
/*        int cmd_status = unit_test_parse(tokens);
    }
*//*}
*/

int main(int argc, char **argv) {
    int num_tokens, i;
    char * buf = malloc(6 * sizeof(char));
    strcpy(argv[0], "hello!");
    // char * str = "";
    // for(i = 0; i < argc; i++) {
    //     char * temp = strcat(str, argv[i]);
    //     str = temp;
    // }
    // printf("%s", str);
    // token ** tokens = tokenize(buf, &num_tokens);

    // PUT WHAT YOU WANT TO TOKENIZE HERE
    test_tokenize("grep Allow < logfile.txt | grep -v google");
    return 0;
}
