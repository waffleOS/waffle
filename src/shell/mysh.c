#include "execute_shell.h"
#include "parse.h"
#include "parser_tests.h"

#include "stdio.h"
#include "unistd.h"

#define INPUT_BUFFER_SIZE 5000

int main(int argc, char **argv) {
    char buf[INPUT_BUFFER_SIZE];

    print_prompt();
    initializeParser();
    while (1) {
        if (fgets(buf, INPUT_BUFFER_SIZE, stdin) != NULL) {
            int num_tokens;
            token ** tokens = tokenize(buf, &num_tokens);
            int num_commands;
            cmd ** cmds = parse(tokens, num_tokens, &num_commands);
            print_commands(cmds, num_commands);
            execute_commands(cmds, num_commands);
            print_prompt();
        }
    }

    return 0;
}
