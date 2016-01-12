#include "execute_shell.h"

#include "stdio.h"
#include "unistd.h"

#define INPUT_BUFFER_SIZE 5000

int main(int argc, char **argv) {
    char buf[INPUT_BUFFER_SIZE];

    print_prompt();
    while (1) {
        if (fgets(buf, INPUT_BUFFER_SIZE, stdin) != NULL) {
            char c = 'H';
            printf("%d", c);
           print_prompt();
        }
    }

    return 0;
}
