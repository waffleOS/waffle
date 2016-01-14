#include "execute_shell.h"
#include "parse.h"
#include "parser_tests.h"

#include "stdio.h"
#include "unistd.h"
#include <stdlib.h>

#define INPUT_BUFFER_SIZE 5000

int main(int argc, char **argv) {
    char buf[INPUT_BUFFER_SIZE];

    print_prompt();
    while (1) {
        if (fgets(buf, INPUT_BUFFER_SIZE, stdin) != NULL) {
            initializeParser();

            // Replace \n in buffer with null
            int i = 0;
            while (buf[i] != '\n')
            {
                i++;
            }
            buf[i] = '\0';

            int num_tokens;

            // Get tokens
            token ** tokens = tokenize(buf, &num_tokens);

            int num_commands;

            // Get commands
            cmd ** cmds = parse(tokens, num_tokens, &num_commands);

            // Execute commands
            execute_commands(cmds, num_commands);

            // Free commands
            for (int j = 0; j < num_commands; j++)
            {
                cmd * cmd = cmds[j];
                free(cmd->input);
                free(cmd->output);
                free(cmd->error);

                // Free each string in argv
                for (int k = 0; k < cmd->argc; k++)
                {
                    free(cmd->argv[k]);
                }
                free(cmd->argv);
                free(cmd);
            }

            // Free cmds buffer
            free(cmds);
            print_prompt();
        }
    }

    return 0;
}
