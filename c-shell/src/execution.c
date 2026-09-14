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

/* Validate all redirect files before forking. Returns 1 if OK, 0 on error. */
int validate_redirections(Redir *redirs, int red_count) {
    for (int i = 0; i < red_count; i++) {
        if (redirs[i].type == 0) {
            /* Input redirect: file must exist and be readable */
            if (access(redirs[i].file, R_OK) != 0) {
                fprintf(stderr, "cshell: no such file or directory\n");
                return 0;
            }
        } else {
            /* Output redirect: try to open/create, then close immediately */
            int flags = (redirs[i].type == 1) ? O_WRONLY|O_CREAT|O_TRUNC : O_WRONLY|O_CREAT|O_APPEND;
            int fd = open(redirs[i].file, flags, 0644);
            if (fd < 0) {
                fprintf(stderr, "cshell: unable to create file for writing\n");
                return 0;
            }
            close(fd);
        }
    }
    return 1;
}

/* Handle multiple redirections: concatenate multiple < inputs, tee multiple > outputs */
static void setup_redirections(Redir *redirs, int red_count) {
    if (red_count == 0) return;

    /* count input and output redirections */
    int in_count = 0, out_count = 0;
    for (int i = 0; i < red_count; i++) {
        if (redirs[i].type == 0) in_count++;
        else out_count++;
    }

    /* --- Input redirections --- */
    if (in_count == 1) {
        for (int i = 0; i < red_count; i++) {
            if (redirs[i].type == 0) {
                int fd = open(redirs[i].file, O_RDONLY);
                if (fd < 0) { fprintf(stderr, "cshell: no such file or directory\n"); _exit(1); }
                dup2(fd, STDIN_FILENO); close(fd);
                break;
            }
        }
    } else if (in_count > 1) {
        int p[2]; pipe(p);
        pid_t cat_pid = fork();
        if (cat_pid == 0) {
            close(p[0]);
            for (int i = 0; i < red_count; i++) {
                if (redirs[i].type != 0) continue;
                int fd = open(redirs[i].file, O_RDONLY);
                if (fd < 0) { fprintf(stderr, "cshell: no such file or directory\n"); _exit(1); }
                char buf[4096]; ssize_t n;
                while ((n = read(fd, buf, sizeof(buf))) > 0) write(p[1], buf, n);
                close(fd);
            }
            close(p[1]); _exit(0);
        }
        close(p[1]);
        dup2(p[0], STDIN_FILENO); close(p[0]);
    }

    /* --- Output redirections --- */
    if (out_count == 1) {
        for (int i = 0; i < red_count; i++) {
            if (redirs[i].type == 0) continue;
            int flags = (redirs[i].type == 1) ? O_WRONLY|O_CREAT|O_TRUNC : O_WRONLY|O_CREAT|O_APPEND;
            int fd = open(redirs[i].file, flags, 0644);
            if (fd < 0) { fprintf(stderr, "cshell: unable to create file for writing\n"); _exit(1); }
            dup2(fd, STDOUT_FILENO); close(fd);
            break;
        }
    } else if (out_count > 1) {
        int p[2]; pipe(p);
        pid_t tee_pid = fork();
        if (tee_pid == 0) {
            close(p[1]);
            int fds[out_count]; int idx = 0;
            for (int i = 0; i < red_count; i++) {
                if (redirs[i].type == 0) continue;
                int flags = (redirs[i].type == 1) ? O_WRONLY|O_CREAT|O_TRUNC : O_WRONLY|O_CREAT|O_APPEND;
                fds[idx] = open(redirs[i].file, flags, 0644);
                if (fds[idx] < 0) { fprintf(stderr, "cshell: unable to create file for writing\n"); _exit(1); }
                idx++;
            }
            char buf[4096]; ssize_t n;
            while ((n = read(p[0], buf, sizeof(buf))) > 0) {
                for (int j = 0; j < idx; j++) write(fds[j], buf, n);
            }
            for (int j = 0; j < idx; j++) close(fds[j]);
            close(p[0]); _exit(0);
        }
        close(p[0]);
        dup2(p[1], STDOUT_FILENO); close(p[1]);
    }
}


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
            if(i==0 && background){
                int has_input_redir=0;
                for(int j=0;j<commands[i].red_count;j++) if(commands[i].redirs[j].type==0) has_input_redir=1;
                if(!has_input_redir){
                    int devnull=open("/dev/null",O_RDONLY);
                    if(devnull>=0){dup2(devnull,STDIN_FILENO);close(devnull);}
                }
            }
            if(i<count-1 && dup2(pipes[i][1],STDOUT_FILENO)<0){
                perror("cshell: dup2");
                exit(EXIT_FAILURE);
            }
            for(int j=0;j<count-1;j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            setup_redirections(commands[i].redirs, commands[i].red_count);

            {
                char *cmd=commands[i].args[0];
                if(cmd[0]=='%'){cmd++;execvp(cmd,commands[i].args);}
                else{if(access(cmd,X_OK)==0)execv(cmd,commands[i].args);execvp(cmd,commands[i].args);}
            }
            fprintf(stderr,"cshell: command not found (%s)\n",commands[i].args[0]);
            _exit(127);
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
        /* Block SIGCHLD so the handler cannot steal foreground waits */
        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);

        tcsetpgrp(STDIN_FILENO,pgid);
        int stopped=0;
        int cmd_not_found=0;
        for(int i=0;i<count;i++){
            int status;
            pid_t result;
            do {
                result=waitpid(pids[i],&status,WUNTRACED);
            } while(result<0 && errno==EINTR);
            if(result>0 && WIFSTOPPED(status))stopped=1;
            if(result>0 && WIFEXITED(status) && WEXITSTATUS(status)==127)cmd_not_found=1;
        }
        tcsetpgrp(STDIN_FILENO,getpgrp());

        /* Unblock SIGCHLD */
        sigprocmask(SIG_SETMASK, &oldmask, NULL);

        if(stopped){
            char *cmd_names[count];
            for(int i=0;i<count;i++)cmd_names[i]=commands[i].args[0];
            int jobn=add_job_group(pgid,pids,cmd_names,count);
            printf("[%d] + Stopped %s\n",jobn,commands[0].args[0]);
        }
        if(cmd_not_found) return -1;
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

            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("cshell: dup2");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        setup_redirections(command->redirs, command->red_count);

        char *command_name = command->args[0];

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

        fprintf(stderr,"cshell: command not found (%s)\n",command->args[0]);

        _exit(127);
    }
    setpgid(pid, pid);

    if (!command->background) {
        /* Block SIGCHLD so the handler cannot steal this foreground wait */
        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);

        tcsetpgrp(STDIN_FILENO,pid);
        int status;
        pid_t result;

        do {
            result=waitpid(pid,&status,WUNTRACED);
        } while(result<0 && errno==EINTR);

        if (result<0 && errno!=ECHILD) {
            perror("cshell: waitpid");
            tcsetpgrp(STDIN_FILENO,getpgrp());
            sigprocmask(SIG_SETMASK, &oldmask, NULL);
            return 0;
        }
        tcsetpgrp(STDIN_FILENO,getpgrp());

        /* Unblock SIGCHLD now that we've reaped the foreground process */
        sigprocmask(SIG_SETMASK, &oldmask, NULL);

        if(result>0 && WIFSTOPPED(status)){
            int jobn=add_job(pid,command->args[0]);
            printf("[%d] + Stopped %s\n",jobn,command->args[0]);
        }
        if(result>0 && WIFEXITED(status) && WEXITSTATUS(status)==127)
            return -1;
    }

    return pid;
}
