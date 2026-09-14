#define _XOPEN_SOURCE 700
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>//file control
#include <dirent.h>      // DIR, opendir, readdir, closedir essentially open directories
#include <sys/stat.h>   // struct stat, stat, S_ISDIR info about filesystem and path
#include <time.h>
#include <math.h>
#include <sys/utsname.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <errno.h>
#include "syscalls.h"

static int peek_reverse(char *filename,int number,int *running_line_no);

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
    time_t *times=NULL;

    if(file!=NULL){
        char line[4096+50];

        while(fgets(line,sizeof(line),file)!=NULL){
            int visits;
            char directory_path[4096];
            long long last_visit_time
            ;

            if(sscanf(line,"%d %lld %4095[^\n]",//path max is supposed to be a thing but whenever i try to implement it says it doesnt exist so i just use 4095
                      &visits,&last_visit_time
                      ,directory_path)!=3){
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

            time_t *new_times=realloc(
                times,
                (count+1)*sizeof(time_t)
            );

            if(new_paths==NULL || new_counts==NULL ||new_times==NULL){
                free(new_paths);
                free(new_counts);
                break;
            }

            paths=new_paths;
            counts=new_counts;
            times=new_times;

            paths[count]=strdup(directory_path);
            counts[count]=visits;
            times[count]=(time_t)last_visit_time;

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
            times[i]=time(NULL);
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

        time_t *new_times=realloc(
            times,
            (count+1)*sizeof(time_t)
        );

        if(new_paths!=NULL && new_counts!=NULL&&new_times!=NULL){
            paths=new_paths;
            counts=new_counts;
            times=new_times;

            paths[count]=strdup(dir_path);

            if(paths[count]!=NULL){
                counts[count]=1;
                times[count]=time(NULL);
                count++;
            }
        }
    }

    file=fopen(path,"w");

    if(file!=NULL){
        for(int i=0;i<count;i++){
            fprintf(file,"%d %lld %s\n",counts[i],(long long)times[i],paths[i]);
        }

        fclose(file);
    }

    for(int i=0;i<count;i++){
        free(paths[i]);
    }

    free(paths);
    free(counts);
    free(path);
    free(times);
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
    double best_rating=-1 ;   
    while(fgets(line,sizeof(line),file)!=NULL){
        int visits;
        long long last_visit;
        char dir_path[4096];

        if(sscanf(line,"%d %lld %4095[^\n]",
                  &visits,&last_visit,dir_path)!=3){
            continue;
        }

        if(strstr(dir_path,name)==NULL){//looks for string in string
            continue;
        }

        if(access(dir_path,F_OK)!=0){//looks if u can access
            continue;
        }
        time_t now=time(NULL);

        double age=difftime(
            now,
            (time_t)last_visit

        );

        // double rating=visits+1000.0/(age+1.0);//to avoid division by zero. complete shit could be so much better,if i keep this recency bias vanishes after 1000 visits
        double rating = visits + 100.0*exp(-age/86400.0);//86400 seconds in a day

        if(rating>best_rating ||
           (rating==best_rating &&
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
            best_rating=rating;
        }
    }

    fclose(file);
    free(path);
    return best;
}

int hop(ShellState *shell,char **args,int count){
    // printf("DEBUG home = [%s]\n", shell->home);
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

static void print_dir(char *path,int all,int recursive,const char *prefix){
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
        int is_dir=(stat(full,&st)==0 && S_ISDIR(st.st_mode));

        if(recursive){
            if(is_dir) printf("%s%s/\n",prefix,names[i]);
            else printf("%s%s\n",prefix,names[i]);
        }else{
            printf("%s\n",names[i]);
        }

        if(recursive && is_dir){
            char sub_prefix[4096];
            snprintf(sub_prefix,sizeof(sub_prefix),"%s%s/",prefix,names[i]);
            print_dir(full,all,recursive,sub_prefix);
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

    print_dir(path,all,recursive,"");
    free(path);
    return 1;
}

static void print_lines(char **lines,int count,int number,int reverse,int *running_line_no){
    if(reverse){
        int nonempty=0;

        if(number){
            for(int i=0;i<count;i++){
                if(lines[i][0]!='\n' && lines[i][0]!='\0')
                    nonempty++;
            }
        }

        int line_no= *running_line_no + nonempty - 1;

        for(int i=count-1;i>=0;i--){
            if(number){
                if(lines[i][0]!='\n' && lines[i][0]!='\0'){
                    printf("%d %s",line_no+1,lines[i]);
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
        *running_line_no += nonempty;
    }
    else{
        for(int i=0;i<count;i++){
            if(number){
                if(lines[i][0]!='\n' && lines[i][0]!='\0'){
                    printf("%d %s",*running_line_no+1,lines[i]);
                    (*running_line_no)++;
                }
                else{
                    printf("%s",lines[i]);
                }
            }
            else printf("%s",lines[i]);
            
        }
    }
}

static int peek_file(char *filename,int number,int reverse,int *running_line_no){
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
        if(reverse && S_ISREG(st.st_mode)) return peek_reverse(filename,number,running_line_no);

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

    print_lines(lines,count,number,reverse,running_line_no);

    for(int i=0;i<count;i++) free(lines[i]);

    free(lines);

    return 1;
}

static int peek_reverse(char *filename,int number,int *running_line_no){
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

    int current_no= *running_line_no + line_no;

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

                    for(int j=line_len-1;j>=0;j--) printf("%c", line[j]);;

                    printf("\n");
                }
                else{
                    printf("\n");
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

        for(int i=line_len-1;i>=0;i--) printf("%c",line[i]);

        printf("\n");
    }

    free(line);
    close(fd);

    *running_line_no += line_no;

    return 1;
}


int peek(char **args,int count){
    int number=0;
    int reverse=0;
    int files=0;
    int running_line_no=0;

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

    if(files==0) return peek_file("-",number,reverse,&running_line_no);

    for(int i=0;i<count;i++){

        if(args[i][0]=='-' && args[i][1]!='\0') continue;

        if(!peek_file(args[i],number,reverse,&running_line_no)) continue;
    }

    return 1;
}

int locate(char **args,int count){
    if(count == 0){
        printf("locate: invalid syntax\n");
        return 0;
    }
    char *path_env=getenv("PATH");

    for(int i=0;i<count;i++){
        int found=0;

        char *cwd=getcwd(NULL,0);

        if(cwd!=NULL){
            char *path=malloc(
                strlen(cwd)+strlen(args[i])+2
            );

            if(path!=NULL){
                sprintf(path,"%s/%s",cwd,args[i]);

                if(access(path,X_OK)==0){
                    // printf("DEBUG: %s\n",path);
                    printf("%s\n",path);
                    found=1;
                }

                free(path);
            }

            free(cwd);
        }

        if(path_env!=NULL){
            char *copy=strdup(path_env);
            char *dir=strtok(copy,":");

            while(dir!=NULL){
                char *path=malloc(
                    strlen(dir)+strlen(args[i])+2
                );

                if(path!=NULL){
                    sprintf(path,"%s/%s",dir,args[i]);

                    if(access(path,X_OK)==0){
                        
                        printf("%s\n",path);
                        found=1;

                    }

                    free(path);
                }

                dir=strtok(NULL,":");
            }

            free(copy);
        }

        if(!found) printf("locate: command not found (%s)\n",args[i]);
    }

    return 1;
}
static void print_spy_row(const char *pid_str, const char *fd, const char *path) {
    struct stat st;
    const char *type_str = "UNKNOWN";
    
    if (stat(path, &st) == 0) {
        switch (st.st_mode & S_IFMT) {
            case S_IFREG: type_str = "REG"; break;
            case S_IFDIR: type_str = "DIR"; break;
            case S_IFCHR: type_str = "CHR"; break;
            case S_IFBLK: type_str = "BLK"; break;
            case S_IFIFO: type_str = "FIFO"; break;
            case S_IFSOCK: type_str = "SOCK"; break;
        }
    }
    printf("%-10s %-10s %-10s %s\n", pid_str, fd, type_str, path);
}

static void print_spy_row_from_link(const char *pid_str, const char *fd, const char *link_path) {
    char target_path[4096];
    ssize_t len = readlink(link_path, target_path, sizeof(target_path) - 1);
    if (len != -1) {
        target_path[len] = '\0';
        print_spy_row(pid_str, fd, target_path);
    }
}

int spy(char **args, int count) {
    if (count > 1) {
        printf("spy: invalid syntax\n");
        return 1;
    }

    pid_t target_pid;
    if (count == 0) {
        target_pid = getpid();
    } else {
        target_pid = atoi(args[0]);
    }

    char proc_dir[256];
    snprintf(proc_dir, sizeof(proc_dir), "/proc/%d", target_pid);

    struct stat st;
    if (stat(proc_dir, &st) != 0) {
        printf("spy: no such process\n");
        return 1;
    }

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", target_pid);

    printf("%-10s %-10s %-10s %s\n", "PID", "FD", "TYPE", "PATH");

    // 1. cwd
    char link_path[1024];
    snprintf(link_path, sizeof(link_path), "%s/cwd", proc_dir);
    print_spy_row_from_link(pid_str, "cwd", link_path);

    // 2. txt (exe)
    snprintf(link_path, sizeof(link_path), "%s/exe", proc_dir);
    print_spy_row_from_link(pid_str, "txt", link_path);

    // 3. mem (mapped files)
    char maps_path[512];
    snprintf(maps_path, sizeof(maps_path), "%s/maps", proc_dir);
    FILE *maps_file = fopen(maps_path, "r");
    if (maps_file) {
        char *line = NULL;
        size_t len = 0;
        char *printed_paths[1024];
        int printed_count = 0;

        while (getline(&line, &len, maps_file) != -1) {
            char path[4096] = {0};
            // format: address perms offset dev inode pathname
            // pathname might be empty
            if (sscanf(line, "%*s %*s %*s %*s %*s %4095[^\n]", path) == 1) {
                if (path[0] == '\0' || path[0] == '[') {
                    continue; // Skip anonymous and special mappings like [heap], [stack], [vdso]
                }
                
                // Check if already printed
                int already_printed = 0;
                for (int i = 0; i < printed_count; i++) {
                    if (strcmp(printed_paths[i], path) == 0) {
                        already_printed = 1;
                        break;
                    }
                }
                
                if (!already_printed) {
                    if (printed_count < 1024) {
                        printed_paths[printed_count++] = strdup(path);
                    }
                    print_spy_row(pid_str, "mem", path);
                }
            }
        }
        free(line);
        fclose(maps_file);
        
        for (int i = 0; i < printed_count; i++) {
            free(printed_paths[i]);
        }
    }

    // 4. numeric fds
    char fd_dir_path[512];
    snprintf(fd_dir_path, sizeof(fd_dir_path), "%s/fd", proc_dir);
    DIR *dir = opendir(fd_dir_path);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] >= '0' && ent->d_name[0] <= '9') {
                snprintf(link_path, sizeof(link_path), "%s/%s", fd_dir_path, ent->d_name);
                print_spy_row_from_link(pid_str, ent->d_name, link_path);
            }
        }
        closedir(dir);
    }

    return 0;
}

static int command_exists(const char *cmd) {
    if (strchr(cmd, '/')) {
        return access(cmd, X_OK) == 0;
    }
    char *path_env = getenv("PATH");
    if (!path_env) return 0;
    
    char *path = strdup(path_env);
    if (!path) return 0;
    
    char *dir = strtok(path, ":");
    char full_path[1024];
    
    while (dir) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
        if (access(full_path, X_OK) == 0) {
            free(path);
            return 1;
        }
        dir = strtok(NULL, ":");
    }
    free(path);
    return 0;
}

struct syscall_entry {
    long scno;
    long count;
    double total_time;
    int first_seen;
    const char *name;
    char dyn_name[32];
};

static int snoop_cmp(const void *a, const void *b) {
    const struct syscall_entry *ea = (const struct syscall_entry *)a;
    const struct syscall_entry *eb = (const struct syscall_entry *)b;
    if (ea->count != eb->count) {
        return (ea->count < eb->count) ? 1 : -1;
    }
    return (ea->first_seen > eb->first_seen) ? 1 : (ea->first_seen < eb->first_seen ? -1 : 0);
}

static volatile sig_atomic_t snoop_interrupted = 0;
static void snoop_sigint_handler(int sig) {
    (void)sig;
    snoop_interrupted = 1;
}

int snoop(char **args, int count) {
    if (count == 0) {
        printf("snoop: invalid syntax\n");
        return 1;
    }

    struct utsname buffer;
    if (uname(&buffer) != 0 || strcmp(buffer.machine, "x86_64") != 0) {
        printf("snoop: unsupported architecture\n");
        return 1;
    }

    int attach_mode = 0;
    pid_t target_pid = -1;

    if (strcmp(args[0], "-p") == 0) {
        if (count != 2) {
            printf("snoop: invalid syntax\n");
            return 1;
        }
        char *endptr;
        long pid_val = strtol(args[1], &endptr, 10);
        if (*endptr != '\0' || pid_val <= 0) {
            printf("snoop: invalid syntax\n");
            return 1;
        }
        target_pid = (pid_t)pid_val;
        attach_mode = 1;

        if (kill(target_pid, 0) == -1) {
            printf("snoop: no such process\n");
            return 1;
        }
    } else {
        if (!command_exists(args[0])) {
            printf("snoop: command not found\n");
            return 1;
        }
    }

    pid_t trace_pid = -1;

    if (attach_mode) {
        if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
            perror("snoop: ptrace attach");
            return 1;
        }
        trace_pid = target_pid;
        int status;
        int ret;
        do {
            ret = waitpid(trace_pid, &status, 0);
        } while (ret == -1 && errno == EINTR);
        if (ret == -1) {
            perror("snoop: waitpid");
            if (ptrace(PTRACE_DETACH, trace_pid, NULL, NULL) == -1) {
                if (errno != ESRCH) perror("snoop: ptrace detach");
            }
            return 1;
        }
        if (!WIFSTOPPED(status)) {
            printf("snoop: process terminated before tracing\n");
            return 1;
        }
    } else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("snoop: fork");
            return 1;
        }
        if (pid == 0) {
            if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
                perror("snoop: ptrace");
                exit(1);
            }
            execvp(args[0], args);
            perror("snoop: execvp");
            exit(1);
        }
        trace_pid = pid;
        int status;
        int ret;
        do {
            ret = waitpid(trace_pid, &status, 0);
        } while (ret == -1 && errno == EINTR);
        if (ret == -1) {
            perror("snoop: waitpid");
            kill(trace_pid, SIGKILL);
            waitpid(trace_pid, &status, 0);
            return 1;
        }
        if (!WIFSTOPPED(status)) {
            return 1;
        }
    }

    if (ptrace(PTRACE_SETOPTIONS, trace_pid, 0, PTRACE_O_TRACESYSGOOD) == -1) {
        perror("snoop: ptrace setoptions");
        if (attach_mode) {
            if (ptrace(PTRACE_DETACH, trace_pid, NULL, NULL) == -1) {
                if (errno != ESRCH) perror("snoop: ptrace detach");
            }
        } else {
            kill(trace_pid, SIGKILL);
            int st; waitpid(trace_pid, &st, 0);
        }
        return 1;
    }

    long counts[MAX_SYSCALL] = {0};
    double times[MAX_SYSCALL] = {0};
    int order[MAX_SYSCALL] = {0};
    long unknown_counts[1024] = {0};
    double unknown_times[1024] = {0};
    long unknown_scnos[1024] = {0};
    int unknown_order[1024] = {0};
    int num_unknown = 0;
    int order_counter = 0;
    int in_syscall = 0;
    long current_syscall = -1;
    struct timespec entry_ts = {0, 0};

    struct sigaction old_sa, new_sa;
    int sigaction_ok = 0;
    snoop_interrupted = 0;
    new_sa.sa_handler = snoop_sigint_handler;
    sigemptyset(&new_sa.sa_mask);
    new_sa.sa_flags = 0;
    if (sigaction(SIGINT, &new_sa, &old_sa) == 0) {
        sigaction_ok = 1;
    } else {
        perror("snoop: sigaction");
    }

    if (attach_mode) {
        printf("snoop: tracing PID %d (press Ctrl-C to stop)\n", trace_pid);
    } else {
        printf("snoop: tracing command %s (press Ctrl-C to stop)\n", args[0]);
    }

    int sig_to_forward = 0;
    
    struct timespec start_time, end_time;
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
        perror("snoop: clock_gettime");
        start_time.tv_sec = 0;
        start_time.tv_nsec = 0;
    }

    while (!snoop_interrupted) {
        if (ptrace(PTRACE_SYSCALL, trace_pid, NULL, sig_to_forward) == -1) {
            if (errno != ESRCH) perror("snoop: ptrace syscall");
            break;
        }
        sig_to_forward = 0;

        int status;
        int ret;
        do {
            ret = waitpid(trace_pid, &status, 0);
        } while (ret == -1 && errno == EINTR && !snoop_interrupted);

        if (ret == -1) {
            if (errno == EINTR && snoop_interrupted) {
                break;
            }
            perror("snoop: waitpid");
            break;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            break;
        }

        if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            if (sig == (SIGTRAP | 0x80)) {
                if (!in_syscall) {
                    struct user_regs_struct regs;
                    if (ptrace(PTRACE_GETREGS, trace_pid, NULL, &regs) == -1) {
                        if (errno != ESRCH) perror("snoop: ptrace getregs");
                        break;
                    }
                    current_syscall = regs.orig_rax;
                    clock_gettime(CLOCK_MONOTONIC, &entry_ts);
                    in_syscall = 1;
                } else {
                    struct timespec exit_ts;
                    clock_gettime(CLOCK_MONOTONIC, &exit_ts);
                    double dur = (exit_ts.tv_sec - entry_ts.tv_sec)
                               + (exit_ts.tv_nsec - entry_ts.tv_nsec) / 1e9;
                    in_syscall = 0;
                    if (current_syscall >= 0 && current_syscall < MAX_SYSCALL) {
                        if (counts[current_syscall] == 0) order[current_syscall] = ++order_counter;
                        counts[current_syscall]++;
                        times[current_syscall] += dur;
                    } else if (current_syscall != -1) {
                        int found = 0;
                        for (int i = 0; i < num_unknown; i++) {
                            if (unknown_scnos[i] == current_syscall) {
                                unknown_counts[i]++; unknown_times[i] += dur; found = 1; break;
                            }
                        }
                        if (!found && num_unknown < 1024) {
                            unknown_scnos[num_unknown] = current_syscall;
                            unknown_counts[num_unknown] = 1;
                            unknown_times[num_unknown] = dur;
                            unknown_order[num_unknown] = ++order_counter;
                            num_unknown++;
                        }
                    }
                    current_syscall = -1;
                }
            } else if (sig == SIGTRAP) {
                // Ignore ordinary SIGTRAP from execve or breakpoint
            } else {
                sig_to_forward = sig;
            }
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
        perror("snoop: clock_gettime");
        end_time = start_time;
    }

    if (sigaction_ok) {
        if (sigaction(SIGINT, &old_sa, NULL) == -1) {
            perror("snoop: sigaction restore");
        }
    }

    if (attach_mode) {
        // Traces until process naturally exits or Ctrl-C interrupts loop.
        if (kill(trace_pid, 0) == 0) {
            if (ptrace(PTRACE_DETACH, trace_pid, NULL, NULL) == -1) {
                if (errno != ESRCH) perror("snoop: ptrace detach");
            } else {
                if (snoop_interrupted) printf("\nsnoop: detached from PID %d\n", trace_pid);
            }
        }
    } else {
        // Unconditionally terminate and reap a command-mode child if it's still alive on exit.
        if (kill(trace_pid, 0) == 0) {
            kill(trace_pid, SIGKILL);
            int st; waitpid(trace_pid, &st, 0);
        }
    }

    int total_unique = 0;
    for (int i = 0; i < MAX_SYSCALL; i++) {
        if (counts[i] > 0) total_unique++;
    }
    total_unique += num_unknown;

    struct syscall_entry *entries = NULL;
    if (total_unique > 0) {
        entries = malloc(total_unique * sizeof(struct syscall_entry));
        if (!entries) {
            perror("snoop: malloc");
            return 1;
        }

        int idx = 0;
        for (int i = 0; i < MAX_SYSCALL; i++) {
            if (counts[i] > 0) {
                entries[idx].scno = i;
                entries[idx].count = counts[i];
                entries[idx].total_time = times[i];
                entries[idx].first_seen = order[i];
                if (syscall_names[i] != NULL) {
                    entries[idx].name = syscall_names[i];
                } else {
                    snprintf(entries[idx].dyn_name, sizeof(entries[idx].dyn_name), "syscall_%d", i);
                    entries[idx].name = entries[idx].dyn_name;
                }
                idx++;
            }
        }
        for (int i = 0; i < num_unknown; i++) {
            entries[idx].scno = unknown_scnos[i];
            entries[idx].count = unknown_counts[i];
            entries[idx].total_time = unknown_times[i];
            entries[idx].first_seen = unknown_order[i];
            snprintf(entries[idx].dyn_name, sizeof(entries[idx].dyn_name), "syscall_%ld", unknown_scnos[i]);
            entries[idx].name = entries[idx].dyn_name;
            idx++;
        }

        qsort(entries, total_unique, sizeof(struct syscall_entry), snoop_cmp);
    }

    printf("%-20s %10s %12s\n", "SYSCALL", "COUNT", "TIME");
    printf("--------------------------------------------\n");
    for (int i = 0; i < total_unique; i++) {
        printf("%-20s %10ld %11.6fs\n", entries[i].name, entries[i].count, entries[i].total_time);
    }
    if (entries) free(entries);

    double elapsed_s = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("\nTotal elapsed time: %.6f seconds\n", elapsed_s);

    return 0;
}
