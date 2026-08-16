#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>// posix function library

int shell_init(ShellState *shell){
    shell->home = getcwd(NULL, 0);
    if (shell->home == NULL) {
        return 0;
    }

    shell->prev = NULL;
    shell->have_prev = 0;

    return 1;
}

void shell_kill(ShellState *shell){
    free(shell->home);
    free(shell->prev);

    shell->home = NULL;
    shell->prev = NULL;
    shell->have_prev = 0;
}


void print_prompt(ShellState *shell){
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return;
    }

    char *username = getenv("USER");

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "?");
    }

    char *display_path = cwd;

    int home_fpath_len = strlen(shell->home);

    if (strncmp(cwd, shell->home, home_fpath_len) == 0 &&
    (cwd[home_fpath_len] == '\0' || cwd[home_fpath_len] == '/')) {
        int suffix_len = strlen(cwd + home_fpath_len);
        char *short_path = malloc(suffix_len + 2);
        if (short_path != NULL) {
            sprintf(short_path, "~%s", cwd + home_fpath_len);
            display_path = short_path;
        }
    }

    printf(
        "<%s@%s:%s> ",
        username,
        hostname,
        display_path
    );

    fflush(stdout);

    if (display_path != cwd) {
        free(display_path);
    }

    free(cwd);
}