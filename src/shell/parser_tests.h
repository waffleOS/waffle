/**
 * parse.h
 * Contains token and cmd structs for the shell.
 */
#ifndef PARSER_TESTS_H
#define PARSER_TESTS_H

void print_tokens(token **tokens, int n);
void print_commands(cmd **commands, int n);
int test_tokenize(char *test_str);
int test_command(token ** tokens, int num_tokens);




#endif /* PARSER_TESTS_H */
