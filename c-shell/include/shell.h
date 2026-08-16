#ifndef SHELL_H
#define SHELL_H

typedef struct {
    char *home;
    char *prev;
    int have_prev;
} ShellState;

typedef enum {
    TOK_WORD,
    TOK_PIPE,
    TOK_AMP,
    TOK_SEMI,
    TOK_LT,
    TOK_GT,
    TOK_GTGT,
    TOK_END
} TokenType;


typedef struct {
    TokenType type;
    char *text;
} Token;

int shell_init(ShellState *shell);
void shell_kill(ShellState *shell);
void print_prompt(ShellState *shell);

#endif
