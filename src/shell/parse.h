/**
 * parse.h
 * Contains token and cmd structs for the shell.
 */
#ifndef PARSE_H
#define PARSE_H

#include "shell_structs.h"

typedef enum {CHAR, QUOTE, REDIRECT, SPACE} token_type;
typedef enum {INITIAL, RECEIVED_CHAR, RECEIVED_QUOTE} state;
typedef void(*f)(char);

typedef struct {
    f action;
    state nextState;
} transition;

token_type getTokenType(char c);
void addChar(char c);
void createToken(char c);
void createTokenRedirect(char c);
void doNOP(char c);

char buffer[1000];
int count;
token tokens[1000];
state currentState;

transition transitions[3][4];


#endif /* PARSE_H */
