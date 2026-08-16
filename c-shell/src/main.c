#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    }

    free(line);// free memory allocated by getline
    shell_kill(&shell);

    return 0;
}

