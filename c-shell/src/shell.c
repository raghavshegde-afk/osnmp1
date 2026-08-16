#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>// posix function library

int shell_init(ShellState *shell){
    //return 0 fail, 1 success
    //on initialization,shell home is set to cwd,no prev exists
    shell->home = getcwd(NULL, 0);
    if (shell->home == NULL) {
        return 0;
    }

    shell->prev = NULL;
    shell->have_prev = 0;

    return 1;
}

void shell_kill(ShellState *shell){
    //free memory allocated for home and prev
    free(shell->home);
    free(shell->prev);

    shell->home = NULL;
    shell->prev = NULL;
    shell->have_prev = 0;
}


void print_prompt(ShellState *shell){
    /* print the shell prompt in the format <username@hostname:current_path>
    if current path is in home directory, replace home directory with ~

    getcwd is here because we must implement hop in b part
    */
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return;
    }//dunno cwd then stop

    char *username = getenv("USER");
    /*
    USER is an environment variable containing the username.
    getenv() returns a pointer to the value stored in the environment; we do NOT free username.
    */

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "?");
    }

    char *disp_path = cwd;

    int home_fpath_len = strlen(shell->home);

    if (strncmp(cwd, shell->home, home_fpath_len) == 0 &&
    (cwd[home_fpath_len] == '\0' || cwd[home_fpath_len] == '/')) {
        // prevents false matches when 2 folders have same start letters
        int suffix_len = strlen(cwd + home_fpath_len);
        char *short_path = malloc(suffix_len + 2);
        if (short_path != NULL) {
            sprintf(short_path, "~%s", cwd + home_fpath_len);
            disp_path = short_path;
        }
    }

    printf(
        "<%s@%s:%s> ",
        username,
        hostname,
        disp_path
    );

    fflush(stdout);//flush stdout to ensure prompt is printed immediately

    if (disp_path != cwd) {//free only if we allocated memory for short_path
        free(disp_path);
    }

    free(cwd);
}