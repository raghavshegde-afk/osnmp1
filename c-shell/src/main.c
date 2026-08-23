#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execution.h"
#include "parser.h"

int main(){

    ShellState shell;
    if (!shell_init(&shell)) {
        fprintf(stderr, "cshell:Failed to initialize shell state\n");
        return 1;
    }
    char *line = NULL;
    size_t capacity = 0;

    while (1) {//run till eof
        print_prompt(&shell);

        long long bytes_read = getline(
            &line,
            &capacity,
            stdin
        );
    /*
     * getline() will allocate memory for the input line.
     * line -> points to the input
     * capacity -> tells getline() how much space is available
     */
        if (bytes_read == -1) {
            putchar('\n');
            break;
        }

        int count = 0;

        Token *tokens = lex_line(line, &count);

        // printf("DEBUG: token count = %d\n", count);

        if (tokens == NULL && count != 0) {
            continue;
        }

        if (!parse_line(tokens, count)) {
            printf("cshell: syntax error\n");
        }
        else {
        //     char *args[count + 1];
        //     int argc = 0;

        //     for (int i = 0; i < count; i++) {
        //         if (tokens[i].type == TOK_WORD) {
        //             args[argc++] = tokens[i].text;
        //         }
        //     }

        //     args[argc] = NULL;

        //     if (argc > 0) {
        //         execute_command(args);
        //     }
        Command command;

        command.argc = 0;
        command.input_file = NULL;
        command.output_file = NULL;
        command.append = 0;

        char *args[count + 1];

        for (int i = 0; i < count; i++) {

            if (tokens[i].type == TOK_WORD) {

                args[command.argc++] = tokens[i].text;

            } 
            else if (tokens[i].type == TOK_LT) {

                if (i + 1 < count &&
                    tokens[i + 1].type == TOK_WORD) {

                    command.input_file = tokens[i + 1].text;
                    i++;
                }

            } 
            else if (tokens[i].type == TOK_GT) {

                if (i + 1 < count &&
                    tokens[i + 1].type == TOK_WORD) {

                    command.output_file = tokens[i + 1].text;
                    command.append = 0;
                    i++;
                }

            } 
            else if (tokens[i].type == TOK_GTGT) {

                if (i + 1 < count &&
                    tokens[i + 1].type == TOK_WORD) {

                    command.output_file = tokens[i + 1].text;
                    command.append = 1;
                    i++;
                }
            }
        }

        args[command.argc] = NULL;
        command.args = args;

        if (command.argc > 0) {
            execute_command(&command);
        }
        }
        

        for (int i = 0; i < count; i++) {
            free(tokens[i].text);
        }

        free(tokens);

        
    }

    free(line);// free memory allocated by getline
    shell_kill(&shell);

    return 0;
}

