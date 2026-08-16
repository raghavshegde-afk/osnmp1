#ifndef SHELL_H
#define SHELL_H

typedef struct {
    char *home;
    char *prev;
    int have_prev;
} ShellState;

typedef enum {
    TOK_WORD,//genuinely any word
    TOK_PIPE,// |
    TOK_AMP,// &
    TOK_SEMI,// ;
    TOK_LT,// <
    TOK_GT,// >
    TOK_GTGT,// >>
    TOK_END// \0
} TokenType;


typedef struct {
    TokenType type;
    char *text;
} Token;

int shell_init(ShellState *shell);
void shell_kill(ShellState *shell);
void print_prompt(ShellState *shell);
Token *lex_line(const char *line, int *count);

#endif
