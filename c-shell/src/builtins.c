#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>//file control
#include <dirent.h>      // DIR, opendir, readdir, closedir essentially open directories
#include <sys/stat.h>   // struct stat, stat, S_ISDIR info about filesystem and path

static int peek_reverse(char *filename,int number);

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

static char *resolve_dir(ShellState *shell,char *name)
{
    char *path;

    if(strcmp(name,"~")==0) return strdup(shell->home);

    if(strcmp(name,"-")==0){
        if(!shell->have_prev)
            return NULL;
        return strdup(shell->prev);
    }

    if(strcmp(name,".")==0 || strcmp(name,"..")==0) return realpath(name,NULL);

    path=realpath(name,NULL);

    return path;
}

static int compare_names(const void *a,const void *b){
    char *const *x=a;
    char *const *y=b;
    return strcmp(*x,*y);
}

static void print_dir(char *path,int all,int recursive){
    DIR *dir=opendir(path);

    if(dir==NULL) return;

    char **names=NULL;
    int count=0;
    struct dirent *entry;

    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0 ||strcmp(entry->d_name,"..")==0)continue;
        
        if(!all && entry->d_name[0]=='.') continue;

        char **new_names=realloc(names,(count+1)*sizeof(char *));//to retain og pointer

        if(new_names==NULL) break;

        names=new_names;
        names[count]=strdup(entry->d_name);

        if(names[count]==NULL) break;

        count++;
    }

    closedir(dir);

    qsort(
        names,
        count,
        sizeof(char *),
        compare_names
    );

    for(int i=0;i<count;i++){
        char full[4096];

        snprintf(
            full,
            sizeof(full),
            "%s/%s",
            path,
            names[i]
        );

        struct stat st;

        if(stat(full,&st)==0 && S_ISDIR(st.st_mode)) printf("%s/\n",names[i]);
        else printf("%s\n",names[i]);
        if(recursive &&
           stat(full,&st)==0 &&
           S_ISDIR(st.st_mode)){
            print_dir(full,all,recursive);
        }

        free(names[i]);//free memory
    }

    free(names);//free pointer
}

int reveal(ShellState *shell,char **args,int count){
    int all=0;
    int recursive=0;
    char *target=NULL;

    for(int i=0;i<count;i++){
        if(args[i][0]=='-' && args[i][1]!='\0'){//start with - is flag,cannot simply exist in vacuum
            for(int j=1;args[i][j]!='\0';j++){

                if(args[i][j]=='a')all=1;
                else if(args[i][j]=='t')recursive=1;
                else{
                    printf("reveal: invalid syntax\n");
                    return 0;
                }
            }
        }
        else{
            if(target!=NULL){
                printf("reveal: invalid syntax\n");
                return 0;
            }

            target=args[i];
        }
    }

    if(target==NULL) target=".";

    char *path=resolve_dir(shell,target);

    if(path==NULL){
        printf("reveal: no such directory\n");
        return 0;
    }

    struct stat st;

    if(stat(path,&st)!=0 || !S_ISDIR(st.st_mode)){
        free(path);
        printf("reveal: no such directory\n");
        return 0;
    }

    print_dir(path,all,recursive);
    free(path);
    return 1;
}

static void print_lines(char **lines,int count,int number,int reverse){
    if(reverse){
        // for(int i=count-1;i>=0;i--){
        //     if(number) printf("%d %s",i+1,lines[i]);
        //     else printf("%s",lines[i]);
        // }
        int line_no=0;

        if(number){
            for(int i=0;i<count;i++){
                if(lines[i][0]!='\n' && lines[i][0]!='\0')
                    line_no++;
            }
        }

        for(int i=count-1;i>=0;i--){
            if(number){
                if(lines[i][0]!='\n' && lines[i][0]!='\0'){
                    printf("%d %s",line_no,lines[i]);
                    line_no--;
                }
                else{
                    printf("%s",lines[i]);
                }
            }
            else{
                printf("%s",lines[i]);
            }
        }
    }
    else{
        int line_no=1;

        for(int i=0;i<count;i++){
            if(number){
                if(lines[i][0]!='\n' && lines[i][0]!='\0'){
                    printf("%d %s",line_no,lines[i]);
                    line_no++;
                }
            }
            else printf("%s",lines[i]);
            
        }
    }
}

static int peek_file(char *filename,int number,int reverse){
    FILE *file;

    if(strcmp(filename,"-")==0) file=stdin;
    else{
        struct stat st;

        
        if(stat(filename,&st)!=0){
            printf("peek: no such file or directory\n");
            return 0;
        }

        if(S_ISDIR(st.st_mode)){
            printf("peek: is a directory\n");
            return 0;
        }
        if(reverse && S_ISREG(st.st_mode)) return peek_reverse(filename,number);

        file=fopen(filename,"r");

        if(file==NULL){
            printf("peek: no such file or directory\n");
            return 0;
        }
    }

    char **lines=NULL;
    int count=0;
    char *line=NULL;
    size_t size=0;

    while(getline(&line,&size,file)!=-1){
        char **new_lines=realloc(
            lines,
            (count+1)*sizeof(char *)
        );

        if(new_lines==NULL){
            free(line);

            for(int i=0;i<count;i++) free(lines[i]);

            free(lines);

            if(file!=stdin) fclose(file);

            return 0;
        }

        lines=new_lines;
        lines[count]=strdup(line);

        if(lines[count]==NULL){
            free(line);

            for(int i=0;i<count;i++) free(lines[i]);

            free(lines);

            if(file!=stdin) fclose(file);

            return 0;
        }

        count++;
    }

    free(line);

    if(file!=stdin) fclose(file);

    print_lines(lines,count,number,reverse);

    for(int i=0;i<count;i++) free(lines[i]);

    free(lines);

    return 1;
}

static int peek_reverse(char *filename,int number)
{
    int fd=open(filename,O_RDONLY);

    if(fd<0){
        printf("peek: no such file or directory\n");
        return 0;
    }

    char buf[4096];
    char *line=NULL;
    int line_len=0;
    int line_no=0;
    int has_char=0;
    off_t end=lseek(fd,0,SEEK_END);

    if(end<0){
        close(fd);
        return 0;
    }

    // count non-empty lines
    if(lseek(fd,0,SEEK_SET)<0){
        close(fd);
        return 0;
    }

    ssize_t n;
    while((n=read(fd,buf,sizeof(buf)))>0){
        for(int i=0;i<n;i++){
            if(buf[i]=='\n'){
                if(has_char) line_no++;
                has_char=0;
            }
            else{
                has_char=1;
            }
        }
    }

    if(has_char) line_no++;

    //read backwards 
    if(lseek(fd,end,SEEK_SET)<0){
        close(fd);
        return 0;
    }

    int current_no=line_no;

    while(end>0){
        // int chunk=end>sizeof(buf) ? sizeof(buf) : end; possibly caused error idk exactly why
        size_t chunk=end>(off_t)sizeof(buf) ? sizeof(buf) : (size_t)end;

        end-=chunk;

        if(lseek(fd,end,SEEK_SET)<0) break;

        n=read(fd,buf,chunk);

        if(n<=0) break;

        for(int i=n-1;i>=0;i--){
            if(buf[i]=='\n'){
                if(line_len>0){
                    if(number) printf("%d ",current_no--);

                    for(int j=line_len-1;j>=0;j--) putchar(line[j]);

                    putchar('\n');
                }
                else{
                    putchar('\n');
                }

                line_len=0;
            }
            else{
                char *new_line=realloc(line,line_len+1);

                if(new_line==NULL){
                    free(line);
                    close(fd);
                    return 0;
                }

                line=new_line;
                line[line_len++]=buf[i];
            }
        }
    }

    if(line_len>0){
        if(number) printf("%d ",current_no);

        for(int i=line_len-1;i>=0;i--) putchar(line[i]);
    }

    free(line);
    close(fd);

    return 1;
}


int peek(char **args,int count){
    int number=0;
    int reverse=0;
    int files=0;

    for(int i=0;i<count;i++){
        if(args[i][0]=='-' && args[i][1]!='\0'){
            for(int j=1;args[i][j]!='\0';j++){
                if(args[i][j]=='n') number=1;
                else if(args[i][j]=='r') reverse=1;
                else{
                    printf("peek: invalid syntax\n");
                    return 0;
                }
            }
        }
        else{
            files++;
        }
    }

    if(files==0) return peek_file("-",number,reverse);

    for(int i=0;i<count;i++){

        if(args[i][0]=='-' && args[i][1]!='\0') continue;

        if(!peek_file(args[i],number,reverse)) return 0;
    }

    return 1;
}