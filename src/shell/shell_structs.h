/**
 * shell_structs.h
 * Contains token and cmd structs for the shell.
 */
#ifndef SHELL_STRUCTS_H
#define SHELL_STRUCTS_H

typedef struct {
    int type;
    char *text;
} token;

typedef struct {
    char *input;  /* NULL iff stdin. */
    char *output; /* NULL iff stdout. */
    char *error;  /* NULL iff stderr. */
    int pipe; /* Whether we need to pipe. */
    int argc;
    char **argv;
} cmd;

#endif /* SHELL_STRUCTS_H */
