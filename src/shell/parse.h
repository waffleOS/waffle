/**
 * parse.h
 * Contains token and cmd structs for the shell.
 */
#ifndef PARSE_H
#define PARSE_H

#include "shell_structs.h"

typedef enum {CHAR, QUOTE, REDIRECT, SPACE} token_type;
typedef void(*f)(char, char *, token *);

token_type getTokenType(char c);
f getTokenValue(token_type token_class);

char buffer[1000];
int count;
token tokens[1000];

#endif /* PARSE_H */
