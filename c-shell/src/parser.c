#include "parser.h"




static int parse_argument(Token *tokens,int count,int *position);
static int parse_command(Token *tokens,int count,int *position);
static int parse_target(Token *tokens,int count,int *position);
static int parse_background(Token *tokens,int count,int *position);

static int parse_command(Token *tokens,int count,int *position){
    if(*position>=count){
        return 0;
    }

    if(tokens[*position].type!=TOK_WORD){
        return 0;
    }

    (*position)++;
    return parse_argument(tokens,count,position);
}

static int parse_target(Token *tokens,int count,int *position){
    if(*position>=count){
        return 0;
    }

    if(tokens[*position].type!=TOK_WORD){
        return 0;
    }

    (*position)++;
    return parse_argument(tokens,count,position);
}

static int parse_background(Token *tokens,int count,int *position){
    if(*position>=count){
        return 1;
    }

    if(tokens[*position].type==TOK_WORD){
        (*position)++;
        return parse_argument(tokens,count,position);
    }

    return 0;
}

static int parse_argument(Token *tokens,int count,int *position){
    if(*position>=count){
        return 1;
    }

    if(tokens[*position].type==TOK_WORD){
        (*position)++;
        return parse_argument(tokens,count,position);
    }

    if(tokens[*position].type==TOK_LT){
        (*position)++;
        return parse_target(tokens,count,position);
    }

    if(tokens[*position].type==TOK_GT){
        (*position)++;
        return parse_target(tokens,count,position);
    }

    if(tokens[*position].type==TOK_GTGT){
        (*position)++;
        return parse_target(tokens,count,position);
    }

    if(tokens[*position].type==TOK_PIPE){
        (*position)++;
        return parse_command(tokens,count,position);
    }

    if(tokens[*position].type==TOK_SEMI){
        (*position)++;
        return parse_command(tokens,count,position);
    }

    if(tokens[*position].type==TOK_AMP){
        (*position)++;
        return parse_background(tokens,count,position);
    }

    return 0;
}

int parse_line(Token *tokens,int count){
    int position=0;

    if(count==0){
        return 1;
    }

    if(tokens[position].type!=TOK_WORD){
        return 0;
    }

    position++;

    if(!parse_argument(tokens,count,&position)){
        return 0;
    }

    return position==count;
}
