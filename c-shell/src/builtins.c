#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *history_path(ShellState *shell){
    char *path=malloc(strlen(shell->home)+20);

    if(path==NULL){
        return NULL;
    }

    strcpy(path,shell->home);
    strcat(path,"/.c-shell_dirs");
    return path;
}

static void note_visit(ShellState *shell,const char *dir_path){
    char *path=history_path(shell);

    if(path==NULL){
        return;
    }

    FILE *file=fopen(path,"r");

    char **paths=NULL;
    int *counts=NULL;
    int count=0;

    if(file!=NULL){
        char line[4096+50];

        while(fgets(line,sizeof(line),file)!=NULL){
            int visits;
            char directory_path[4096];

            if(sscanf(line,"%d %4095[^\n]",
                      &visits,directory_path)!=2){
                continue;
            }

            char **new_paths=realloc(
                paths,
                (count+1)*sizeof(char *)
            );

            int *new_counts=realloc(
                counts,
                (count+1)*sizeof(int)
            );

            if(new_paths==NULL || new_counts==NULL){
                free(new_paths);
                free(new_counts);
                break;
            }

            paths=new_paths;
            counts=new_counts;

            paths[count]=strdup(directory_path);
            counts[count]=visits;

            if(paths[count]==NULL){
                break;
            }

            count++;
        }

        fclose(file);
    }

    int found=0;

    for(int i=0;i<count;i++){
        if(strcmp(paths[i],dir_path)==0){
            counts[i]++;
            found=1;
            break;
        }
    }

    if(!found){
        char **new_paths=realloc(
            paths,
            (count+1)*sizeof(char *)
        );

        int *new_counts=realloc(
            counts,
            (count+1)*sizeof(int)
        );

        if(new_paths!=NULL && new_counts!=NULL){
            paths=new_paths;
            counts=new_counts;

            paths[count]=strdup(dir_path);

            if(paths[count]!=NULL){
                counts[count]=1;
                count++;
            }
        }
    }

    file=fopen(path,"w");

    if(file!=NULL){
        for(int i=0;i<count;i++){
            fprintf(file,"%d %s\n",counts[i],paths[i]);
        }

        fclose(file);
    }

    for(int i=0;i<count;i++){
        free(paths[i]);
    }

    free(paths);
    free(counts);
    free(path);
}

static int ch_dir(ShellState *shell,char *path){
    char *current=getcwd(NULL,0);

    if(current==NULL){
        return 0;
    }

    if(chdir(path)!=0){
        free(current);
        return 0;
    }

    free(shell->prev);
    shell->prev=current;
    shell->have_prev=1;
    char *new_cwd=getcwd(NULL,0);

    if(new_cwd!=NULL){
        note_visit(shell,new_cwd);
        free(new_cwd);
    }
    return 1;
}

static char *find_frecency_match(ShellState *shell,char *name){
    char *path=history_path(shell);

    if(path==NULL){
        return NULL;
    }

    FILE *file=fopen(path,"r");

    if(file==NULL){
        free(path);
        return NULL;
    }

    char line[4096+50];

    char *best=NULL;
    int best_count=-1;

    while(fgets(line,sizeof(line),file)!=NULL){
        int visits;
        char dir_path[4096];

        if(sscanf(line,"%d %4095[^\n]",
                  &visits,dir_path)!=2){
            continue;
        }

        if(strstr(dir_path,name)==NULL){//looks for string in string
            continue;
        }

        if(access(dir_path,F_OK)!=0){//looks if u can access
            continue;
        }

        if(visits>best_count ||
           (visits==best_count &&
            (best==NULL || strcmp(dir_path,best)<0))){

            char *new_best=malloc(strlen(dir_path)+1);

            if(new_best==NULL){
                free(best);
                best=NULL;
                break;
            }

            strcpy(new_best,dir_path);
            free(best);
            best=new_best;
            best_count=visits;
        }
    }

    fclose(file);
    free(path);
    return best;
}

int hop(ShellState *shell,char **args,int count){
    if(count==0){
        if(!ch_dir(shell,shell->home)){
            printf("hop: no such directory\n");
        }
        return 1;
    }

    for(int i=0;i<count;i++){

        if(strcmp(args[i],"~")==0){
            if(!ch_dir(shell,shell->home)){
                printf("hop: no such directory\n");
            }
        }
        else if(strcmp(args[i],".")==0){
            continue;
        }
        else if(strcmp(args[i],"..")==0){
            if(!ch_dir(shell,"..")){
                printf("hop: no such directory\n");
            }
        }
        else if(strcmp(args[i],"-")==0){
            if(shell->have_prev){
                if(!ch_dir(shell,shell->prev)){
                    printf("hop: no such directory\n");
                }
            }
        }
        else{
            if(!ch_dir(shell,args[i])){
                char *match=find_frecency_match(shell,args[i]);

                if(match==NULL){
                    printf("hop: no such directory\n");
                }else{
                    if(!ch_dir(shell,match)){
                        printf("hop: no such directory\n");
                    }
                    free(match);
                }
            }
        }
    }

    return 1;
}

