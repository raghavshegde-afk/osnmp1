#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>

int execute_command(Command *command){
    if (command == NULL ||
        command->args == NULL ||
        command->args[0] == NULL) {
        return 0;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("cshell: fork");
        return 0;
    }

    if (pid == 0) {

        // input redirection < 
        if (command->input_file != NULL) {

            int fd = open(
                command->input_file,
                O_RDONLY
            );

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

        // output redirection > or >> 
        if (command->output_file != NULL) {

            int flags = O_WRONLY | O_CREAT;

            if (command->append) {
                flags |= O_APPEND;
            }
            else {
                flags |= O_TRUNC;
            }

            int fd = open(
                command->output_file,
                flags,
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

            close(fd);
        }

        execvp(command->args[0],command->args);

        //execvp only lets program flow go forward if execution fails
        fprintf(
            stderr,
            "cshell: command not found (%s)\n",
            command->args[0]
        );

        exit(EXIT_FAILURE);
    }

    if (waitpid(pid, NULL, 0) < 0) {
        perror("cshell: waitpid");
        return 0;
    }

    return 1;
}