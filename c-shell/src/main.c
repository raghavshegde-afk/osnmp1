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
#include <unistd.h>

static volatile sig_atomic_t child_exited=0;
// volatile sig_atomic_t foreground_pid=0;

void handle_sigchld(int sig){
    (void)sig;
    int saved_errno=errno;
    int status;
    pid_t pid;
    int reaped = 0;

    while((pid=waitpid(-1,&status,WNOHANG))>0){
        record_child_exit(pid,status);
        reaped = 1;
    }
    if (reaped) child_exited=1;
    errno=saved_errno;
}


int main(){
    struct sigaction sa;
    sa.sa_handler=handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGCHLD,&sa,NULL);

    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);

    setpgid(0,0);
    tcsetpgrp(STDIN_FILENO,getpgrp());

    ShellState shell;
    if (!shell_init(&shell)) {
        fprintf(stderr, "cshell:Failed to initialize shell state\n");
        return 1;
    }
    char *line = NULL;
    size_t capacity = 0;
    int prompt_redrawn_for_sigchld = 0;
    int eof_warned = 0;

    while (1) {//run till eof
        int had_child_exit = child_exited;

        if(had_child_exit){
            child_exited=0;
            reap_jobs();
        }
        if(!had_child_exit || !prompt_redrawn_for_sigchld){
            print_prompt(&shell);
            if(had_child_exit)prompt_redrawn_for_sigchld=1;
        }



read_again:
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
                if (!child_exited) goto read_again;
                continue;
            }
            if(has_stopped_jobs()){
                if(!eof_warned){
                    eof_warned=1;
                    printf("cshell: there are stopped jobs\n");
                    clearerr(stdin);
                    continue;
                }
            }
            putchar('\n');
            break;
        }

        eof_warned=0;
        if(feof(stdin)){
            clearerr(stdin);
        }

        prompt_redrawn_for_sigchld=0;

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
                int start=0;
                while(start<count){
                    int end=start;
                    while(end<count && tokens[end].type!=TOK_AMP)end++;
                Command command;

                command.argc = 0;
                command.redirs = NULL;
                command.red_count = 0;
                command.background=(end<count && tokens[end].type==TOK_AMP);

                char *args[end-start+1];

                for (int i = start; i < end; i++) {

                    if (tokens[i].type == TOK_WORD) {
                        args[command.argc++] = tokens[i].text;
                    }
                    else if (
                        tokens[i].type == TOK_LT ||
                        tokens[i].type == TOK_GT ||
                        tokens[i].type == TOK_GTGT
                    ) {

                        if (
                            i+1<end &&
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
                    else if (strcmp(command.args[0], "activities") == 0) {
                        print_activities();
                    }
                    else if (strcmp(command.args[0], "fg") == 0) {
                        if (command.argc != 2) {
                            fprintf(stderr, "cshell: fg: invalid number of arguments\n");
                        } else {
                            int job_number = atoi(command.args[1]);
                            Job *job = get_job_by_number(job_number);
                            if (!job) {
                                fprintf(stderr, "cshell: fg: %d: no such job\n", job_number);
                            } else {
                                pid_t pgid = job->pgid;
                                int p_count = job->process_count;
                                pid_t pids[MAX_PIPELINE_PROCS];
                                char cmd_names[MAX_PIPELINE_PROCS][256];
                                char first_cmd_name[256];
                                strcpy(first_cmd_name, job->job_name);
                                for (int i = 0; i < p_count; i++) {
                                    pids[i] = job->pids[i];
                                    strcpy(cmd_names[i], job->command_names[i]);
                                }
                                
                                remove_job(pgid);
                                tcsetpgrp(STDIN_FILENO, pgid);
                                kill(-pgid, SIGCONT);
                                
                                int stopped = 0;
                                for (int i = 0; i < p_count; i++) {
                                    int status;
                                    pid_t result;
                                    do {
                                        result = waitpid(pids[i], &status, WUNTRACED);
                                    } while (result < 0 && errno == EINTR);
                                    if (result > 0 && WIFSTOPPED(status)) stopped = 1;
                                }
                                
                                tcsetpgrp(STDIN_FILENO, getpgrp());
                                
                                if (stopped) {
                                    int jobn;
                                    if (p_count == 1) {
                                        jobn = add_job(pgid, first_cmd_name);
                                    } else {
                                        char *name_ptrs[MAX_PIPELINE_PROCS];
                                        for (int i = 0; i < p_count; i++) name_ptrs[i] = cmd_names[i];
                                        jobn = add_job_group(pgid, pids, name_ptrs, p_count);
                                    }
                                    printf("[%d] + Stopped    %s\n", jobn, first_cmd_name);
                                }
                            }
                        }
                    }
                    else if (strcmp(command.args[0], "bg") == 0) {
                        if (command.argc != 2) {
                            fprintf(stderr, "cshell: bg: invalid number of arguments\n");
                        } else {
                            int job_number = atoi(command.args[1]);
                            Job *job = get_job_by_number(job_number);
                            if (!job) {
                                fprintf(stderr, "cshell: bg: %d: no such job\n", job_number);
                            } else {
                                kill(-job->pgid, SIGCONT);
                            }
                        }
                    }
                    else if (strcmp(command.args[0], "ping") == 0) {
                        int is_valid_signal = 1;
                        if (command.argc != 3) {
                            is_valid_signal = 0;
                        } else {
                            char *sig_str = command.args[2];
                            if (sig_str[0] == '\0') is_valid_signal = 0;
                            for (int i = 0; sig_str[i] != '\0'; i++) {
                                if (sig_str[i] < '0' || sig_str[i] > '9') {
                                    is_valid_signal = 0;
                                    break;
                                }
                            }
                        }

                        if (!is_valid_signal) {
                            printf("ping: invalid syntax\n");
                        } else {
                            int sig_num = atoi(command.args[2]);
                            int actual_sig = sig_num % 64;
                            char *target = command.args[1];
                            
                            if (target[0] == '%') {
                                int job_num = atoi(target + 1);
                                Job *job = get_job_by_number(job_num);
                                if (!job) {
                                    printf("ping: no such process found\n");
                                } else {
                                    kill(-job->pgid, actual_sig);
                                    printf("Sent signal %d to %s\n", sig_num, target);
                                }
                            } else {
                                pid_t pid = atoi(target);
                                if (!is_tracked_pid(pid)) {
                                    printf("ping: no such process found\n");
                                } else {
                                    kill(pid, actual_sig);
                                    printf("Sent signal %d to %s\n", sig_num, target);
                                }
                            }
                        }
                    }
                    else {
                        pid_t pid=execute_command(&command);
                        if(command.background){
                            // int jobn = add_job(pid);
                            int jobn = add_job(pid,command.args[0]);
                            printf("[%d] %d\n",jobn,pid);
                        }
                    }
                }

                free(command.redirs);
                start=end+1;
                }
            }

            else {
                int pipeline_count=1;
                int background=count>0 && tokens[count-1].type==TOK_AMP;
                int token_end=background ? count-1 : count;

                for(int i=0;i<token_end;i++){
                    if(tokens[i].type==TOK_PIPE)pipeline_count++;
                }

                Command commands[pipeline_count];
                char *args[pipeline_count][count+1];
                int start=0;
                int valid=1;

                for(int stage=0;stage<pipeline_count;stage++){
                    int end=start;
                    while(end<token_end && tokens[end].type!=TOK_PIPE)end++;

                    commands[stage].argc=0;
                    commands[stage].redirs=NULL;
                    commands[stage].red_count=0;
                    commands[stage].background=background;

                    for(int i=start;i<end;i++){
                        if(tokens[i].type==TOK_WORD){
                            args[stage][commands[stage].argc++]=tokens[i].text;
                        }
                        else if(tokens[i].type==TOK_LT || tokens[i].type==TOK_GT || tokens[i].type==TOK_GTGT){
                            if(i+1<end && tokens[i+1].type==TOK_WORD){
                                Redir *r=realloc(commands[stage].redirs,(commands[stage].red_count+1)*sizeof(Redir));

                                if(r==NULL){
                                    free(commands[stage].redirs);
                                    commands[stage].redirs=NULL;
                                    valid=0;
                                    break;
                                }

                                commands[stage].redirs=r;
                                commands[stage].redirs[commands[stage].red_count].file=tokens[i+1].text;
                                if(tokens[i].type==TOK_LT)commands[stage].redirs[commands[stage].red_count].type=0;
                                else if(tokens[i].type==TOK_GT)commands[stage].redirs[commands[stage].red_count].type=1;
                                else commands[stage].redirs[commands[stage].red_count].type=2;
                                commands[stage].red_count++;
                                i++;
                            }
                        }
                    }

                    args[stage][commands[stage].argc]=NULL;
                    commands[stage].args=args[stage];
                    if(commands[stage].argc==0)valid=0;
                    start=end+1;
                }

                if(valid){
                    pid_t out_pids[pipeline_count];
                    pid_t pid=execute_pipeline(commands,pipeline_count,background,out_pids);
                    if(background && pid>0){
                        char *cmd_names[pipeline_count];
                        for(int i=0;i<pipeline_count;i++)cmd_names[i]=commands[i].args[0];
                        int jobn=add_job_group(pid,out_pids,cmd_names,pipeline_count);
                        printf("[%d] %d\n",jobn,pid);
                    }
                }

                for(int i=0;i<pipeline_count;i++)free(commands[i].redirs);
            }
        }

    

        for (int i = 0; i < count; i++) {
            free(tokens[i].text);
        }

        free(tokens);

        
    }

    kill_all_jobs();
    free(line);// free memory allocated by getline
    shell_kill(&shell);

    return 0;
}
