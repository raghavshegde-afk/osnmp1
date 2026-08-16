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

char *make_word(const char *line, int *index, int *error)
{
    int capacity=16;
    int length=0;

    char *word=malloc(capacity);

    if (word==NULL) {
        *error=1;
        return NULL;
    }

    while (line[*index]!='\0') {

        if (is_space_ch(line[*index]) ||
            is_special_ch(line[*index])) {
            break;
        }

        if (length+1>=capacity) {
            capacity*=2;

            char *new_word=realloc(word, capacity);

            if (new_word==NULL) {
                free(word);
                *error=1;
                return NULL;
            }

            word=new_word;
        }

        word[length]=line[*index];
        length++;
        (*index)++;
    }

    word[length]='\0';

    return word;
}

Token *lex_line(const char *line, int *count)
{
    Token *tokens = NULL;
    *count = 0;

    int index = 0;

    while (line[index] != '\0') {
        if (is_space_ch(line[index])) {
            index++;
            continue;
        }
        if (line[index]=='|') {
            tokens = add_token(tokens,count,TOK_PIPE,NULL);

            if (tokens==NULL) {
                return NULL;
            }

            index++;
            continue;
        }
        if (line[index]=='&') {
            tokens = add_token(tokens,count,TOK_AMP,NULL);

            if (tokens==NULL) {
                return NULL;
            }

            index++;
            continue;
        }
        if (line[index]==';') {
            tokens = add_token(tokens,count,TOK_SEMI,NULL);

            if (tokens==NULL) {
                return NULL;
            }

            index++;
            continue;
        }
        if (line[index]=='<') {
            tokens = add_token(tokens,count,TOK_LT,NULL);

            if (tokens==NULL) {
                return NULL;
            }

            index++;
            continue;
        }
        if (line[index]=='>') {
            if(line[index + 1]=='>'){
                tokens = add_token(tokens,count,TOK_GTGT,NULL);
                index++;
            }else {
                tokens = add_token(tokens,count,TOK_GT,NULL);
            }

            if(tokens == NULL) {
                return NULL;
            }

            index++;
            continue;
        }
        // int start = index;

        // while (line[index] != '\0' && !is_space_ch(line[index]) &&
        // !is_special_ch(line[index])) {
        //     index++;
        // }

        // int length = index - start;

        // char *word = malloc(length + 1);

        // if (word == NULL) {
        //     free(tokens);
        //     return NULL;
        // }

        // memcpy(
        //     word,
        //     line + start,
        //     length
        // );

        // word[length] = '\0';

        // tokens = add_token(tokens,count,TOK_WORD,word);
        
    }

    return tokens;
}