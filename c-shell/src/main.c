#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execution.h"
#include "parser.h"
#include "builtins.h"

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
            printf("cshell: invalid syntax\n");
        }
        else {
        //initial code
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

        // Code for just redirection    
        // Command command;

        // command.argc = 0;
        // command.input_file = NULL;
        // command.output_file = NULL;
        // command.append = 0;

        // char *args[count + 1];

        // for (int i = 0; i < count; i++) {

        //     if (tokens[i].type == TOK_WORD) {

        //         args[command.argc++] = tokens[i].text;

        //     } 
        //     else if (tokens[i].type == TOK_LT) {

        //         if (i + 1 < count &&
        //             tokens[i + 1].type == TOK_WORD) {

        //             command.input_file = tokens[i + 1].text;
        //             i++;
        //         }

        //     } 
        //     else if (tokens[i].type == TOK_GT) {

        //         if (i + 1 < count &&
        //             tokens[i + 1].type == TOK_WORD) {

        //             command.output_file = tokens[i + 1].text;
        //             command.append = 0;
        //             i++;
        //         }

        //     } 
        //     else if (tokens[i].type == TOK_GTGT) {

        //         if (i + 1 < count &&
        //             tokens[i + 1].type == TOK_WORD) {

        //             command.output_file = tokens[i + 1].text;
        //             command.append = 1;
        //             i++;
        //         }
        //     }
        // }

        // args[command.argc] = NULL;
        // command.args = args;

        // // if (command.argc > 0) {
        // //     execute_command(&command);
        // // }
        // if (command.argc > 0) {

        //     if (strcmp(command.args[0], "hop") == 0) {
        //         hop(
        //             &shell,
        //             &command.args[1],
        //             command.argc - 1
        //         );
        //     }
        //     else {
        //         execute_command(&command);
        //     }
        // }

        //     Command command;

        //     command.argc = 0;
        //     command.redirs = NULL;
        //     command.red_count = 0;

        //     char *args[count + 1];

        //     for (int i = 0; i < count; i++) {

        //         if (tokens[i].type == TOK_WORD) {
        //             args[command.argc++] = tokens[i].text;
        //         }

        //         else if (tokens[i].type == TOK_LT ||tokens[i].type == TOK_GT ||tokens[i].type == TOK_GTGT) {

        //             if (i + 1 < count &&tokens[i + 1].type == TOK_WORD) {

        //                 Redir *new_redirs = realloc(
        //                     command.redirs,
        //                     (command.red_count + 1) * sizeof(Redir)
        //                 );

        //                 if (new_redirs == NULL) {
        //                     free(command.redirs);
        //                     command.redirs = NULL;
        //                     break;
        //                 }

        //                 command.redirs = new_redirs;

        //                 command.redirs[command.red_count].file =tokens[i + 1].text;

        //                 if (tokens[i].type == TOK_LT)command.redirs[command.red_count].type = 0;
        //                 else if (tokens[i].type == TOK_GT)command.redirs[command.red_count].type = 1;
        //                 else command.redirs[command.red_count].type = 2;

        //                 command.red_count++;
        //                 i++;
        //             }
        //         }
        //     }

        //     args[command.argc] = NULL;
        //     command.args = args;

        //     if (command.argc > 0) {
        //         if (strcmp(command.args[0], "hop") == 0) {
        //             hop(
        //                 &shell,
        //                 &command.args[1],
        //                 command.argc - 1
        //             );
        //         }
        //         else {
        //             execute_command(&command);
        //         }
        //     }

        //     free(command.redirs);
        // }

            int pipe_pos = -1;

            for (int i = 0; i < count; i++) {
                if (tokens[i].type == TOK_PIPE) {
                    pipe_pos = i;
                    break;
                }
            }

            if (pipe_pos == -1) {
                Command command;

                command.argc = 0;
                command.redirs = NULL;
                command.red_count = 0;

                char *args[count + 1];

                for (int i = 0; i < count; i++) {

                    if (tokens[i].type == TOK_WORD) {
                        args[command.argc++] = tokens[i].text;
                    }

                    else if (
                        tokens[i].type == TOK_LT ||
                        tokens[i].type == TOK_GT ||
                        tokens[i].type == TOK_GTGT
                    ) {

                        if (
                            i + 1 < count &&
                            tokens[i + 1].type == TOK_WORD
                        ) {

                            Redir *r = realloc(
                                command.redirs,
                                (command.red_count + 1) * sizeof(Redir)
                            );

                            if (r == NULL) {
                                free(command.redirs);
                                command.redirs = NULL;
                                break;
                            }

                            command.redirs = r;

                            command.redirs[command.red_count].file =
                                tokens[i + 1].text;

                            if (tokens[i].type == TOK_LT) {
                                command.redirs[command.red_count].type = 0;
                            }

                            else if (tokens[i].type == TOK_GT) {
                                command.redirs[command.red_count].type = 1;
                            }

                            else {
                                command.redirs[command.red_count].type = 2;
                            }

                            command.red_count++;
                            i++;
                        }
                    }
                }

                args[command.argc] = NULL;
                command.args = args;

                if (command.argc > 0) {

                    if (strcmp(command.args[0], "hop") == 0) {
                        hop(
                            &shell,
                            &command.args[1],
                            command.argc - 1
                        );
                    }

                    else {
                        execute_command(&command);
                    }
                }

                free(command.redirs);
            }

            else {
                Command left;
                Command right;

                left.argc = 0;
                left.redirs = NULL;
                left.red_count = 0;

                right.argc = 0;
                right.redirs = NULL;
                right.red_count = 0;

                char *left_args[pipe_pos + 1];
                char *right_args[count - pipe_pos];

                // for (int i = 0; i < pipe_pos; i++) {

                //     if (tokens[i].type == TOK_WORD) {
                //         left_args[left.argc++] = tokens[i].text;
                //     }
                // }

                // for (int i = pipe_pos + 1; i < count; i++) {

                //     if (tokens[i].type == TOK_WORD) {
                //         right_args[right.argc++] = tokens[i].text;
                //     }
                // }
                for (int i = 0; i < pipe_pos; i++) {

                    if (tokens[i].type == TOK_WORD) {
                        left_args[left.argc++] = tokens[i].text;
                    }

                    else if (
                        tokens[i].type == TOK_LT ||
                        tokens[i].type == TOK_GT ||
                        tokens[i].type == TOK_GTGT
                    ) {

                        if (
                            i + 1 < pipe_pos &&
                            tokens[i + 1].type == TOK_WORD
                        ) {

                            Redir *r = realloc(
                                left.redirs,
                                (left.red_count + 1) * sizeof(Redir)
                            );

                            if (r == NULL) {
                                free(left.redirs);
                                left.redirs = NULL;
                                break;
                            }

                            left.redirs = r;

                            left.redirs[left.red_count].file =
                                tokens[i + 1].text;

                            if (tokens[i].type == TOK_LT) {
                                left.redirs[left.red_count].type = 0;
                            }

                            else if (tokens[i].type == TOK_GT) {
                                left.redirs[left.red_count].type = 1;
                            }

                            else {
                                left.redirs[left.red_count].type = 2;
                            }

                            left.red_count++;
                            i++;
                        }
                    }
                }

                for (int i = pipe_pos + 1; i < count; i++) {

                    if (tokens[i].type == TOK_WORD) {
                        right_args[right.argc++] = tokens[i].text;
                    }

                    else if (
                        tokens[i].type == TOK_LT ||
                        tokens[i].type == TOK_GT ||
                        tokens[i].type == TOK_GTGT
                    ) {

                        if (
                            i + 1 < count &&
                            tokens[i + 1].type == TOK_WORD
                        ) {

                            Redir *r = realloc(
                                right.redirs,
                                (right.red_count + 1) * sizeof(Redir)
                            );

                            if (r == NULL) {
                                free(right.redirs);
                                right.redirs = NULL;
                                break;
                            }

                            right.redirs = r;

                            right.redirs[right.red_count].file =
                                tokens[i + 1].text;

                            if (tokens[i].type == TOK_LT) {
                                right.redirs[right.red_count].type = 0;
                            }

                            else if (tokens[i].type == TOK_GT) {
                                right.redirs[right.red_count].type = 1;
                            }

                            else {
                                right.redirs[right.red_count].type = 2;
                            }

                            right.red_count++;
                            i++;
                        }
                    }
                }

                left_args[left.argc] = NULL;
                right_args[right.argc] = NULL;

                left.args = left_args;
                right.args = right_args;

                if (
                    left.argc > 0 &&
                    right.argc > 0
                ) {
                    execute_pipe(&left, &right);
                }

                free(left.redirs);
                free(right.redirs);
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

