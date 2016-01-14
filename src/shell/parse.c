
#include "parse.h"
#include "stdio.h"
#include "unistd.h"
#include <stdlib.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 5000

void initializeParser()
{
    buffer = malloc(1000 * sizeof(char));
    tokens = malloc(1000 * sizeof(token *));
    cmds = malloc(1000 * sizeof(cmd *));

    // Initialize FSM
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
    // Check if we need to create a token
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
    // Get the transition given the current state and type of token
    transition t = transitions[currentState][token_class];

    // Execute the transition
    t.action(c);

    // Update the current state to the next state
    currentState = t.nextState;
}

void addChar(char c)
{
    // Write c to the buffer
    buffer[count++] = c;
}

void createToken(char c)
{
    // Add a null termination to the buffer
    buffer[count++] = '\0';

    // Create a token
    token *t = (token *) malloc(sizeof(token));

    // Set the token type to text to indicate that this is not a redirection
    t->type = TEXT;

    // Set the token text
    t->text = malloc(count * sizeof(char));
    strcpy(t->text, buffer);

    // Set the token length to the length of the text
    t->length = count;

    // Store the token pointer
    tokens[tokenCount++] = t;

    // Reset the buffer index
    count = 0;
}

void createTokenAndRedirect(char c)
{
    // Create text token
    createToken(c);

    // Create redirection token
    createRedirectToken(c);
}

void createRedirectToken(char c)
{
    // Create a token
    token * t = (token *) malloc(sizeof(token));

    // Set the token type to redirection
    t->type = REDIRECT_TOKEN;

    // Set the token text to the redirection character with null termination
    t->text = malloc(2 * sizeof(char));
    t->text[0] = c;
    t->text[1] = '\0';

    // Set the token length to the length of the text
    t->length = 2;

    // Store the token pointer
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

            // If we have not created a command for the current tokens
            if (newCommand)
            {
                // Initialize an array of strings
                char ** argv = (char **) malloc((argc + 1) * sizeof(char *));
                int j;
                int k = 0;

                // Start at the first token in the set and add the token text
                // to the array
                for (j = i - argc; j < i; j++)
                {
                    token x = *tokens[j];
                    argv[k] = (char *) malloc(x.length);
                    strcpy(argv[k], x.text);
                    k++;
                }
                // Add a NULL token to the end of the array.
                argv[k] = NULL;

                // Create a token
                c = (cmd *) malloc(sizeof(cmd));
                c->input = NULL;
                c->output = NULL;
                c->error = NULL;
                c->argc = argc;
                c->argv = argv;
                cmds[cmdCount++] = c;

                // Set newCommand to 0 since we do not need to create a command
                newCommand = 0;
                argc = 0;
            }
        }

        // If the last token was a "<", set the stdin of the previous command
        else if (setPreviousInput)
        {
            cmds[cmdCount - 1]->input = malloc(t.length * sizeof(char));
            strcpy(cmds[cmdCount - 1]->input, t.text);
            setPreviousInput = 0;
        }

        // If the last token was a ">", set the stdout of the previous command
        else if (setPreviousOutput)
        {
            cmds[cmdCount - 1]->output = malloc(t.length * sizeof(char));
            strcpy(cmds[cmdCount - 1]->output, t.text);
            setPreviousOutput = 0;
        }

        // Move to the next token
        else
        {
            // Update the number of tokens seen so far since the last command
            argc++;
            // Set newCommand to 1 since we can create a command
            newCommand = 1;
        }
    }

    // Create the last command if newCommand is 1
    if (newCommand)
    {
        // Initialize an array of strings
        char ** argv = (char **) malloc((argc + 1) * sizeof(char *));
        int j;
        int k = 0;

        // Start at the first token in the set and add the token text
        // to the array
        for (j = i - argc; j < i; j++)
        {
            token x = *tokens[j];
            argv[k] = (char *) malloc(x.length);
            strcpy(argv[k], x.text);
            k++;
            free(tokens[j]->text);
            free(tokens[j]);
        }
        // Add a NULL token to the end of the array.
        argv[k] = NULL;

        // Create a token
        c = (cmd *) malloc(sizeof(cmd));
        c->argc = argc;
        c->argv = argv;
        c->input = NULL;
        c->output = NULL;
        cmds[cmdCount++] = c;
        argc = 0;
    }

    *num_commands = cmdCount;
    free(buffer);
    free(tokens);
    free(cmds);
    return cmds;
}
