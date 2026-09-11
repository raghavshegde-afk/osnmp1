#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execution.h"
#include "parser.h"
#include "builtins.h"
#include "jobs.h"
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

static volatile sig_atomic_t child_exited=0;
// volatile sig_atomic_t foreground_pid=0;

void handle_sigchld(int sig){
    (void)sig;
    // while(waitpid(-1,NULL,WNOHANG)>0);
    // child_exited=1;
    child_exited=1;
}


int main(){
    signal(SIGCHLD,handle_sigchld);
    ShellState shell;
    if (!shell_init(&shell)) {
        fprintf(stderr, "cshell:Failed to initialize shell state\n");
        return 1;
    }
    char *line = NULL;
    size_t capacity = 0;

    while (1) {//run till eof
        if(child_exited){
            // pid_t pid;
            // while((pid=waitpid(-1,NULL,WNOHANG))>0)remove_job(pid);
            reap_jobs();
            child_exited=0;
        }
        // print_prompt(&shell);
        if(!child_exited)print_prompt(&shell);



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
        if(bytes_read==-1){
            if(errno==EINTR){
                clearerr(stdin);
                continue;
            }
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
            int semi_pos = -1;

            for (int i = 0; i < count; i++) {
                if (tokens[i].type == TOK_SEMI) {
                    semi_pos = i;
                    break;
                }
            
            }
            int amp_pos=-1;

            for (int i=0;i<count;i++) {
                if (tokens[i].type==TOK_AMP) {
                    amp_pos=i;
                    break;
                }
            }

            int pipe_pos = -1;

            for (int i = 0; i < count; i++) {
                if (tokens[i].type == TOK_PIPE) {
                    pipe_pos = i;
                    break;
                }
            }
            if (semi_pos!=-1){
                // Command command;
                // command.argc=0;
                // command.redirs=NULL;
                // command.red_count=0;

                // char *args[semi_pos+1];

                // for (int i=0;i<semi_pos;i++) {
                //     if (tokens[i].type==TOK_WORD) {
                //         args[command.argc++]=tokens[i].text;
                //     }
                // }

                // args[command.argc]=NULL;
                // command.args=args;

                // if (command.argc>0)execute_command(&command);

                // free(command.redirs);

                // Command command2;
                // command2.argc=0;
                // command2.redirs=NULL;
                // command2.red_count=0;

                // char *args2[count-semi_pos];

                // for (int i=semi_pos+1;i<count;i++) {
                //     if (tokens[i].type==TOK_WORD)args2[command2.argc++]=tokens[i].text;
                // }

                // args2[command2.argc]=NULL;
                // command2.args=args2;

                // if (command2.argc>0)execute_command(&command2);

                // free(command2.redirs);


                int start=0;

                while (start<count) {
                    int end=start;

                    while(end<count && tokens[end].type!=TOK_SEMI)end++;

                    Command command;
                    command.argc=0;
                    command.redirs=NULL;
                    command.red_count=0;
                    command.background=0;

                    char *args[end-start+1];

                    for (int i=start;i<end;i++) {
                        if (tokens[i].type==TOK_WORD)
                            args[command.argc++]=tokens[i].text;
                    }

                    args[command.argc]=NULL;
                    command.args=args;

                    if (command.argc>0)execute_command(&command);

                    free(command.redirs);

                    start=end+1;
                }
            }
            else if (pipe_pos==-1) {
                Command command;

                command.argc = 0;
                command.redirs = NULL;
                command.red_count = 0;
                command.background=0;
                if (amp_pos!=-1)command.background=1;

                char *args[count + 1];

                for (int i = 0; i < count; i++) {

                    if (tokens[i].type == TOK_WORD) {
                        args[command.argc++] = tokens[i].text;
                    }
                    else if(tokens[i].type==TOK_AMP){
                        command.background=1;
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
                    else if (strcmp(command.args[0], "reveal") == 0) {
                        reveal(
                            &shell,
                            &command.args[1],
                            command.argc - 1
                        );
                    }

                    else if (strcmp(command.args[0], "peek") == 0) {
                        peek(
                            &command.args[1],
                            command.argc - 1
                        );
                    }

                    else if (strcmp(command.args[0], "locate") == 0) {
                        locate(
                            &command.args[1],
                            command.argc - 1
                        );
                    }
                    else {
                        pid_t pid=execute_command(&command);
                        if(command.background){
                            int jobn = add_job(pid);
                            printf("[%d] %d\n",jobn,pid);
                        }
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
