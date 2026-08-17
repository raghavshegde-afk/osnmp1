#include "builtins.h"
#include <stdio.h>

int main(void)
{
    char *a[]={"test.txt"};
    char *b[]={"-n","test.txt"};
    char *c[]={"-r","test.txt"};
    char *d[]={"-rn","test.txt"};
    char *e[]={"missing.txt"};
    char *f[]={ "." };

    printf("=== PEEK ===\n");

    printf("\n--- normal ---\n");
    peek(a,1);

    printf("\n--- -n ---\n");
    peek(b,2);

    printf("\n--- -r ---\n");
    peek(c,2);

    printf("\n--- -rn ---\n");
    peek(d,2);

    printf("\n--- missing ---\n");
    peek(e,1);

    printf("\n--- directory ---\n");
    peek(f,1);

    return 0;
}