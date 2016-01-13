
#include "parse.h"
#include "stdio.h"
#include "unistd.h"
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 5000

// int main(int argc, char **argv)
// {
//     int num_tokens;
//     char * buf = malloc(6 * sizeof(char));
//     strcpy(argv[0], "hello!");
//     token ** tokens = tokenize(buf, &num_tokens);
// }

void initializeParser()
{
    buffer = malloc(1000 * sizeof(char));
    tokens = malloc(1000 * sizeof(token *));
    cmds = malloc(1000 * sizeof(cmd *));
    transitions[INITIAL][CHAR] = (transition) {&addChar, RECEIVED_CHAR};
    transitions[INITIAL][QUOTE] = (transition) {&doNOP, RECEIVED_QUOTE};
    transitions[INITIAL][REDIRECT] = (transition) {&createRedirectToken, INITIAL};
    transitions[INITIAL][SPACE] = (transition) {&doNOP, INITIAL};

    transitions[RECEIVED_CHAR][CHAR] = (transition) {&addChar, RECEIVED_CHAR};
    transitions[RECEIVED_CHAR][QUOTE] = (transition) {&doNOP, RECEIVED_QUOTE};
    transitions[RECEIVED_CHAR][REDIRECT] = (transition) {&createTokenAndRedirect, INITIAL};
    transitions[RECEIVED_CHAR][SPACE] = (transition) {&createToken, INITIAL};

    transitions[RECEIVED_QUOTE][CHAR] = (transition) {&addChar, RECEIVED_QUOTE};
    transitions[RECEIVED_QUOTE][QUOTE] = (transition) {&doNOP, RECEIVED_CHAR};
    transitions[RECEIVED_QUOTE][REDIRECT] = (transition) {&addChar, RECEIVED_QUOTE};
    transitions[RECEIVED_QUOTE][SPACE] = (transition) {&addChar, RECEIVED_QUOTE};

}

token ** tokenize(char * buf, int * num_tokens)
{
    count = 0;
    tokenCount = 0;
    currentState = INITIAL;
    token_type lastSeen;
    while (*buf != '\0')
    {
        char c = *buf;
        token_type t = getTokenType(c);
        lastSeen = t;
        executeTransition(c, t);
        buf++;
    }
    if (lastSeen != REDIRECT && lastSeen != SPACE)
    {
        createToken(' ');
    }

    *num_tokens = tokenCount;
    return tokens;
}

token_type getTokenType(char c)
{
    if (c == '"') return QUOTE;
    else if (c == '|' || c == '<' || c == '>') return REDIRECT;
    else if (c == ' ') return SPACE;
    else return CHAR;
}

void executeTransition(char c, token_type token_class)
{
    transition t = transitions[currentState][token_class];
    t.action(c);
    currentState = t.nextState;
}

void addChar(char c)
{
    buffer[count++] = c;
}

void createToken(char c)
{
    buffer[count++] = '\0';
    token *t = (token *) malloc(sizeof(token));
    t->type = TEXT;
    t->text = malloc(count * sizeof(char));
    strcpy(t->text, buffer);
    t->length = count;
    tokens[tokenCount++] = t;
    count = 0;
}

void createTokenAndRedirect(char c)
{
    createToken(c);
    createRedirectToken(c);
}

void createRedirectToken(char c)
{
    token * t = (token *) malloc(sizeof(token));
    t->type = REDIRECT_TOKEN;
    t->text = malloc(2 * sizeof(char));
    t->text[0] = c;
    t->text[1] = '\0';
    t->length = 2;
    tokens[tokenCount++] = t;
}

void doNOP(char c)
{

}

cmd ** parse(token ** tokens, int num_tokens, int * num_commands)
{
    int cmdCount = 0;
    int i;
    cmd * c;
    int argc = 0;
    int setPreviousInput = 0;
    int setPreviousOutput = 0;
    int newCommand = 0;
    for (i = 0; i < num_tokens; i++)
    {
        token t = *tokens[i];
        if (t.type == REDIRECT_TOKEN)
        {
            if (strncmp(t.text, "<", 1) == 0)
            {
                setPreviousInput = 1;
            }
            else if (strncmp(t.text, ">", 1) == 0)
            {
                setPreviousOutput = 1;
            }

            if (newCommand)
            {
                char ** argv = (char **) malloc((argc + 1) * sizeof(char *));
                int j;
                int k = 0;
                for (j = i - argc; j < i; j++)
                {
                    token x = *tokens[j];
                    argv[k] = (char *) malloc(x.length);
                    strcpy(argv[k], x.text);
                    k++;
                }
                argv[k] = NULL;
                c = (cmd *) malloc(sizeof(cmd));
                c->input = NULL;
                c->output = NULL;
                c->error = NULL;
                c->argc = argc;
                c->argv = argv;
                cmds[cmdCount++] = c;
                newCommand = 0;
                argc = 0;
            }
        }

        else if (setPreviousInput)
        {
            cmds[cmdCount - 1]->input = malloc(t.length * sizeof(char));
            strcpy(cmds[cmdCount - 1]->input, t.text);
            setPreviousInput = 0;
        }

        else if (setPreviousOutput)
        {
            cmds[cmdCount - 1]->output = malloc(t.length * sizeof(char));
            strcpy(cmds[cmdCount - 1]->output, t.text);
            setPreviousOutput = 0;
        }
        else
        {
            argc++;
            newCommand = 1;
        }
    }

    if (newCommand)
    {
        char ** argv = (char **) malloc(argc * sizeof(char *));
        int j;
        int k = 0;
        for (j = i - argc; j < i; j++)
        {
            token x = *tokens[j];
            argv[k] = (char *) malloc(x.length);
            strcpy(argv[k], x.text);
            k++;
        }
        c = (cmd *) malloc(sizeof(cmd));
        c->argc = argc;
        c->argv = argv;
        c->input = NULL;
        c->output = NULL;
        cmds[cmdCount++] = c;
        argc = 0;
    }

    *num_commands = cmdCount;
    return cmds;
}
