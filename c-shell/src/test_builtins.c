#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void show_cwd(void)
{
    char *cwd=getcwd(NULL,0);

    if(cwd!=NULL){
        printf("CWD: %s\n",cwd);
        free(cwd);
    }
}

int main(void)
{
    ShellState shell;

    if(!shell_init(&shell))
        return 1;

    printf("=== B1 HOP ===\n");

    show_cwd();

    char *up[]={".."};
    hop(&shell,up,1);
    show_cwd();

    char *back[]={"-"};
    hop(&shell,back,1);
    show_cwd();

    char *home[]={"~"};
    hop(&shell,home,1);
    show_cwd();

    printf("\n=== B2 REVEAL ===\n");

    char *none[]={};
    reveal(&shell,none,0);

    printf("\n--- reveal -a ---\n");

    char *all[]={"-a"};
    reveal(&shell,all,1);

    printf("\n--- reveal -t ---\n");

    char *recursive[]={"-t"};
    reveal(&shell,recursive,1);

    printf("\n--- reveal -ta ---\n");

    char *both[]={"-ta"};
    reveal(&shell,both,1);

    printf("\n--- invalid flag ---\n");

    char *bad[]={"-x"};
    reveal(&shell,bad,1);

    printf("\n--- nonexistent directory ---\n");

    char *missing[]={"does_not_exist"};
    reveal(&shell,missing,1);

    shell_kill(&shell);

    return 0;
}