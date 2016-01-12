
#include "parse.h"
#include "stdio.h"
#include "unistd.h"

#define INPUT_BUFFER_SIZE 5000

int main(int argc, char **argv)
{
    // printf("%d", getTokenType(' '));
}

void initializeParser()
{
    count = 0;
    transitions[INITIAL][CHAR] = (transition) {&addChar, RECEIVED_CHAR};
    transitions[INITIAL][QUOTE] = (transition) {&doNOP, RECEIVED_QUOTE};
    transitions[INITIAL][REDIRECT] = (transition) {&createToken, INITIAL};
    transitions[INITIAL][SPACE] = (transition) {&doNOP, INITIAL};

    transitions[RECEIVED_CHAR][CHAR] = (transition) {&addChar, RECEIVED_CHAR};
    transitions[RECEIVED_CHAR][QUOTE] = (transition) {&doNOP, RECEIVED_QUOTE};
    transitions[RECEIVED_CHAR][REDIRECT] = (transition) {&createTokenRedirect, INITIAL};
    transitions[RECEIVED_CHAR][SPACE] = (transition) {&createToken, INITIAL};

    transitions[RECEIVED_QUOTE][CHAR] = (transition) {&addChar, RECEIVED_QUOTE};
    transitions[RECEIVED_QUOTE][QUOTE] = (transition) {&doNOP, RECEIVED_CHAR};
    transitions[RECEIVED_QUOTE][REDIRECT] = (transition) {&addChar, RECEIVED_QUOTE};
    transitions[RECEIVED_QUOTE][SPACE] = (transition) {&addChar, RECEIVED_QUOTE};

}

token_type getTokenType(char c)
{
    if (c == '"') return QUOTE;
    else if (c == '|' || c == '<' || c == '>') return REDIRECT;
    else if (c == ' ') return SPACE;
    else return CHAR;
}

void executeTransition(state current, char c, token_type token_class)
{
    transition t = transitions[current][token_class];
    t.action(c);
    currentState = t.nextState;
}

void addChar(char c)
{

}

void createToken(char c)
{

}

void createTokenRedirect(char c)
{

}

void doNOP(char c)
{

}
/*
FSM
token ** tokenize(char * buf, int & num_tokens)
{

}
*/
