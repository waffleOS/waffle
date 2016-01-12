/*******************************************************************************
 * parser_tests.c 
 * CS124 Team Waffle Assignment 1 Shell
 * parser_tests.c
 * 
 * This file includes functions to test the tokenize and parsing functions.
 ******************************************************************************/

/* Print out tokens in array. */ 
void print_tokens(token **tokens, int n) {
    int i;
    for(i = 0; i < n; i++) {
        printf("Token %d of type %d: %s\n", i, type, tokens[i]->text);
    }
}

/* Print out commands in array. */ 
void print_commands(cmd **commands, int n) {
    int i, j;
    for(i = 0; i < n; i++) {
        printf("Command %d with %d arguments. Arguments:\n", i, cmd[i]->argc);
        for(j = 0; j < cmd[i]->argc; j++) {
            printf("%s\n", (cmd[i]->argv)[j]);
        }

        if(input == NULL) {
            printf("Input is stdin.\n");
        } else {
            printf("Input is %s.\n", input);
        }

        if(output == NULL) {
            printf("Output is stdout.\n");
        } else {
            printf("Output is %s.\n", output);
        }

        if(error == NULL) {
            printf("Error is stderr.\n");
        } else {
            printf("Error is %s.\n", error);
        }
    }

}


/*
 * Test tokenize function and returns status of test. 
 * token **tokenize(char *buf, int *num_tokens);
 */
int test_tokenize(char *test_str, int expected_num_tkns) {
    printf("%d tokens for test string:\n", expected_num_tkns, test_str);
    token **tkns = tokenize(test_str, &expected_num_tkns);
    printf("Tokenize called. Returned:\n");
    print_tokens(tkns, expected_num_tkns);
}

/* 
 * Unit test parse function
 * Tests the parse function given a specified
 * array of tokens and returns the status of the test.
 */
int unit_test_parse(token **tokens); 

/* Integration test of tokenize and parse. */
int int_test_parse(char *test_str) {
    /* Sample implementation */
    int token_status = test_tokenize(test_str);
    if (token_status == TOKEN_GOOD) {
        /* TODO: set up tokens using tokenize and a buffer. */
        int cmd_status = unit_test_parse(tokens);
    }
}