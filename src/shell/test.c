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
#include "parser_tests.h"

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
    test_tokenize("echo \"hello\"");
    //test_tokenize("echo \"hello\"");
    return 0;
}
