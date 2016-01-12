
#include "parse.h"
#include "stdio.h"
#include "unistd.h"

#define INPUT_BUFFER_SIZE 5000

f tokenValues[4] = { 1, 2, 3, 4 };

int main(int argc, char **argv)
{
    printf("%d", getTokenType(' '));
}

void initializeParser()
{
    count = 0;
}

token_type getTokenType(char c)
{
    if (c == '"') return QUOTE;
    else if (c == '|' || c == '<' || c == '>') return REDIRECT;
    else if (c == ' ') return SPACE;
    else return CHAR;
}

f getTokenValue(token_type token_class)
{

}

void addChar(char c)
{

}
/*
FSM
token ** tokenize(char * buf, int & num_tokens)
{

}
*/
