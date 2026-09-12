#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "jobs.h"



// extern volatile sig_atomic_t foreground_pid;

// int execute_command(Command *command){
//     if (command == NULL ||
//         command->args == NULL ||
//         command->args[0] == NULL) {
//         return 0;
//     }

//     pid_t pid = fork();

//     if (pid < 0) {
//         perror("cshell: fork");
//         return 0;
//     }

//     if (pid == 0) {

//         // input redirection < 
//         if (command->input_file != NULL) {

//             int fd = open(
//                 command->input_file,
//                 O_RDONLY
//             );

//             if (fd < 0) {
//                 perror("cshell");
//                 exit(EXIT_FAILURE);
//             }

//             if (dup2(fd, STDIN_FILENO) < 0) {
//                 perror("cshell: dup2");
//                 close(fd);
//                 exit(EXIT_FAILURE);
//             }

//             close(fd);
//         }

//         // output redirection > or >> 
//         if (command->output_file != NULL) {

//             int flags = O_WRONLY | O_CREAT;

//             if (command->append) {
//                 flags |= O_APPEND;
//             }
//             else {
//                 flags |= O_TRUNC;
//             }

//             int fd = open(
//                 command->output_file,
//                 flags,
//                 0644
//             );

//             if (fd < 0) {
//                 perror("cshell");
//                 exit(EXIT_FAILURE);
//             }

//             if (dup2(fd, STDOUT_FILENO) < 0) {
//                 perror("cshell: dup2");
//                 close(fd);
//                 exit(EXIT_FAILURE);
//             }

//             close(fd);
//         }

//         execvp(command->args[0],command->args);

//         //execvp only lets program flow go forward if execution fails
//         fprintf(
//             stderr,
//             "cshell: command not found (%s)\n",
//             command->args[0]
//         );

//         exit(EXIT_FAILURE);
//     }

//     if (waitpid(pid, NULL, 0) < 0) {
//         perror("cshell: waitpid");
//         return 0;
//     }

//     return 1;
// }

pid_t execute_pipeline(Command *commands, int count, int background, pid_t *out_pids){
    if(count<2)return 0;

    int pipes[count-1][2];
    pid_t pids[count];
    pid_t pgid=0;

    for(int i=0;i<count-1;i++){
        if(pipe(pipes[i])<0){
            perror("cshell: pipe");
            while(i-- > 0){
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            return 0;
        }
    }

    for(int i=0;i<count;i++){
        pids[i]=fork();

        if(pids[i]<0){
            perror("cshell: fork");
            for(int j=0;j<count-1;j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            while(i-- > 0)waitpid(pids[i],NULL,0);
            return 0;
        }

        if(pids[i]==0){
            setpgid(0,i==0 ? 0 : pgid);
            signal(SIGINT,SIG_DFL);
            signal(SIGTSTP,SIG_DFL);

            if(i>0 && dup2(pipes[i-1][0],STDIN_FILENO)<0){
                perror("cshell: dup2");
                exit(EXIT_FAILURE);
            }
            if(i<count-1 && dup2(pipes[i][1],STDOUT_FILENO)<0){
                perror("cshell: dup2");
                exit(EXIT_FAILURE);
            }
            for(int j=0;j<count-1;j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            for(int j=0;j<commands[i].red_count;j++){
                int fd;

                if(commands[i].redirs[j].type==0)
                    fd=open(commands[i].redirs[j].file,O_RDONLY);
                else if(commands[i].redirs[j].type==1)
                    fd=open(commands[i].redirs[j].file,O_WRONLY|O_CREAT|O_TRUNC,0644);
                else
                    fd=open(commands[i].redirs[j].file,O_WRONLY|O_CREAT|O_APPEND,0644);

                if(fd<0){
                    perror("cshell");
                    exit(EXIT_FAILURE);
                }
                if(commands[i].redirs[j].type==0)dup2(fd,STDIN_FILENO);
                else dup2(fd,STDOUT_FILENO);
                close(fd);
            }

            execvp(commands[i].args[0],commands[i].args);
            fprintf(stderr,"cshell: %s: command not found\n",commands[i].args[0]);
            exit(EXIT_FAILURE);
        }

        if(i==0)pgid=pids[i];
        setpgid(pids[i],pgid);
    }

    for(int i=0;i<count-1;i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if(out_pids){
        for(int i=0;i<count;i++)out_pids[i]=pids[i];
    }

    if(!background){
        tcsetpgrp(STDIN_FILENO,pgid);
        int stopped=0;
        for(int i=0;i<count;i++){
            int status;
            waitpid(pids[i],&status,WUNTRACED);
            if(WIFSTOPPED(status))stopped=1;
        }
        tcsetpgrp(STDIN_FILENO,getpgrp());
        if(stopped){
            char *cmd_names[count];
            for(int i=0;i<count;i++)cmd_names[i]=commands[i].args[0];
            int jobn=add_job_group(pgid,pids,cmd_names,count);
            printf("[%d] + Stopped    %s\n",jobn,commands[0].args[0]);
        }
    }

    return pids[0];
}

pid_t execute_command(Command *command){
    if (!command || !command->args || !command->args[0])
        return 0;

    pid_t pid = fork();

    if (pid < 0) {
        perror("cshell: fork");
        return 0;
    }

    if (pid == 0) {
        setpgid(0, 0);
        signal(SIGINT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);

        if (command->background) {
            int fd = open("/dev/null", O_RDONLY);

            if (fd < 0) {
                perror("cshell");
                exit(EXIT_FAILURE);
            }

            // dup2(fd, STDIN_FILENO);
            // close(fd);
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("cshell: dup2");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        for (int i = 0; i < command->red_count; i++) {
            int fd;

            if (command->redirs[i].type == 0) {
                fd = open(command->redirs[i].file, O_RDONLY);

                if (fd < 0) {
                    perror("cshell");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("cshell: dup2");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            else if (command->redirs[i].type == 1) {
                fd = open(
                    command->redirs[i].file,
                    O_WRONLY | O_CREAT | O_TRUNC,
                    0644
                );

                if (fd < 0) {
                    perror("cshell");
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("cshell: dup2");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            else {
                fd = open(
                    command->redirs[i].file,
                    O_WRONLY | O_CREAT | O_APPEND,
                    0644
                );

                if (fd < 0) {
                    perror("cshell");
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("cshell: dup2");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }

            close(fd);
        }

        // execvp(command->args[0], command->args);
        char *command_name = command->args[0];

        // if (command_name[0] == '%') {
        //     command_name++;
        // }

        // execvp(command_name, command->args);

        if (command_name[0] == '%') {
            command_name++;
            execvp(command_name, command->args);
        }
        else {
            if (access(command_name, X_OK) == 0) {
                execv(command_name, command->args);
            }
            execvp(command_name, command->args);
        }

        fprintf(stderr,"cshell: %s: command not found\n",command->args[0]);

        exit(EXIT_FAILURE);
    }
    setpgid(pid, pid);

    // if(!command->background)foreground_pid=pid;
    if (!command->background) {
        tcsetpgrp(STDIN_FILENO,pid);
        int status;
        pid_t result;

        do {
            result=waitpid(pid,&status,WUNTRACED);
        } while(result<0 && errno==EINTR);

        if (result<0 && errno!=ECHILD) {
            perror("cshell: waitpid");
            tcsetpgrp(STDIN_FILENO,getpgrp());
            return 0;
        }
        tcsetpgrp(STDIN_FILENO,getpgrp());
        if(result>0 && WIFSTOPPED(status)){
            int jobn=add_job(pid,command->args[0]);
            printf("[%d] + Stopped    %s\n",jobn,command->args[0]);
        }
        // foreground_pid=0;
    }

    return pid;
}
