#ifndef EXECUTION_H
#define EXECUTION_H

// int execute_command(char **args);

typedef struct {
    char **args;
    int argc;

    char *input_file;
    char *output_file;

    int append;
} Command;

int execute_command(Command *command);

int execute_pipe(Command *left, Command *right);

#endif