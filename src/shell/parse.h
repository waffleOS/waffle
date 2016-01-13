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
void createTokenAndRedirect(char c);
void createRedirectToken(char c);
void doNOP(char c);
void executeTransition(char c, token_type token_class);
token * tokenize(char * buf, int * num_tokens);
void initializeParser();
cmd * parse(token * tokens, int num_tokens, int * num_commands);

char * buffer;
int count;
int tokenCount;
token * tokens;
cmd * cmds;
state currentState;

transition transitions[3][4];


#endif /* PARSE_H */
