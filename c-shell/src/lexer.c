#include "shell.h"
#include <stdlib.h>
#include <string.h>

Token *add_token(Token *tokens,int *count,TokenType type,char *text){
    Token *new_tokens = realloc(
        tokens,
        (*count + 1) * sizeof(Token)
    );

    if (new_tokens == NULL) {
        free(text);
        free(tokens);
        return NULL;
    }

    new_tokens[*count].type = type;
    new_tokens[*count].text = text;

    *count += 1;

    return new_tokens;
}

int is_space_ch(char c){
    return c == ' ' ||
           c == '\t' ||
           c == '\n' ||
           c == '\r';
}

int is_special_ch(char c){
    return c == '|' ||
           c == '&' ||
           c == ';' ||
           c == '<' ||
           c == '>';
}