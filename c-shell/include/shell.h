#ifndef SHELL_H
#define SHELL_H

typedef struct {
    char *home;
    char *prev;
    int have_prev;
} ShellState;

int shell_init(ShellState *shell);
void shell_kill(ShellState *shell);
void print_prompt(ShellState *shell);

#endif
