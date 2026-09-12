#include "jobs.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define MAX_JOBS 64
#define MAX_JOB_EVENTS 128

static Job jobs[MAX_JOBS];
static int job_count=0;
static int next_job_number=1;
static volatile sig_atomic_t event_pids[MAX_JOB_EVENTS];
static volatile sig_atomic_t event_statuses[MAX_JOB_EVENTS];
static volatile sig_atomic_t event_head=0;
static volatile sig_atomic_t event_tail=0;

int add_job(pid_t pid,char *command_name){
    if (job_count>=MAX_JOBS)return -1;
    jobs[job_count].job_number=next_job_number++;
    jobs[job_count].pgid=pid;
    snprintf(jobs[job_count].job_name,sizeof(jobs[job_count].job_name),"%s",command_name);
    jobs[job_count].process_count=1;
    jobs[job_count].pids[0]=pid;
    snprintf(jobs[job_count].command_names[0],sizeof(jobs[job_count].command_names[0]),"%s",command_name);
    int number=jobs[job_count].job_number;
    job_count++;
    return number;
}

int add_job_group(pid_t pgid, pid_t *pids, char **names, int count){
    if(job_count>=MAX_JOBS)return -1;
    if(count>MAX_PIPELINE_PROCS)count=MAX_PIPELINE_PROCS;
    jobs[job_count].job_number=next_job_number++;
    jobs[job_count].pgid=pgid;
    snprintf(jobs[job_count].job_name,sizeof(jobs[job_count].job_name),"%s",names[0]);
    jobs[job_count].process_count=count;
    for(int i=0;i<count;i++){
        jobs[job_count].pids[i]=pids[i];
        snprintf(jobs[job_count].command_names[i],sizeof(jobs[job_count].command_names[i]),"%s",names[i]);
    }
    int number=jobs[job_count].job_number;
    job_count++;
    return number;
}

void remove_job(pid_t pid){
    for (int i=0;i<job_count;i++) {
        if (jobs[i].pgid==pid) {
            for (int j=i;j<job_count-1;j++)jobs[j]=jobs[j+1];
            job_count--;
            return;
        }
    }
}

void print_jobs(){
    for (int i=0;i<job_count;i++)printf("[%d] %d\n",jobs[i].job_number,jobs[i].pgid);
}

void record_child_exit(pid_t pid, int status){
    sig_atomic_t next=(event_head+1)%MAX_JOB_EVENTS;

    if(next!=event_tail){
        event_pids[event_head]=(sig_atomic_t)pid;
        event_statuses[event_head]=(sig_atomic_t)status;
        event_head=next;
    }
}

void reap_jobs(void){
    sigset_t set,oldset;

    sigemptyset(&set);
    sigaddset(&set,SIGCHLD);
    sigprocmask(SIG_BLOCK,&set,&oldset);

    while(event_tail!=event_head){
        pid_t pid=(pid_t)event_pids[event_tail];
        int status=(int)event_statuses[event_tail];

        event_tail=(event_tail+1)%MAX_JOB_EVENTS;

        for(int i=0;i<job_count;i++){
            int found=0;
            for(int p=0;p<jobs[i].process_count;p++){
                if(jobs[i].pids[p]==pid){
                    found=1;
                    /* Remove this process from the job's pid list */
                    for(int k=p;k<jobs[i].process_count-1;k++){
                        jobs[i].pids[k]=jobs[i].pids[k+1];
                        memcpy(jobs[i].command_names[k],jobs[i].command_names[k+1],256);
                    }
                    jobs[i].process_count--;
                    break;
                }
            }
            if(found){
                if(jobs[i].process_count==0){
                    /* All processes done – print completion using pgid */
                    pid_t pgid=jobs[i].pgid;
                    if(WIFEXITED(status) && WEXITSTATUS(status)==0)
                        printf("%s with pid %d exited normally\n",jobs[i].job_name,pgid);
                    else
                        printf("%s with pid %d exited abnormally\n",jobs[i].job_name,pgid);
                    remove_job(pgid);
                }
                break;
            }
        }
    }

    sigprocmask(SIG_SETMASK,&oldset,NULL);
}

void print_activities(void){
    for(int i=0;i<job_count;i++){
        printf("[%d] pgid %d\n",jobs[i].job_number,jobs[i].pgid);
        for(int p=0;p<jobs[i].process_count;p++){
            pid_t pid=jobs[i].pids[p];
            /* Check if process is alive */
            if(kill(pid,0)<0)continue; /* process gone */

            /* Determine state: read /proc/<pid>/stat */
            char path[64];
            snprintf(path,sizeof(path),"/proc/%d/stat",pid);
            FILE *f=fopen(path,"r");
            char state_str[16]="Running";
            if(f){
                int tmp_pid;
                char comm[256];
                char state;
                if(fscanf(f,"%d %255s %c",&tmp_pid,comm,&state)==3){
                    if(state=='T' || state=='t')
                        snprintf(state_str,sizeof(state_str),"Stopped");
                }
                fclose(f);
            }
            printf("%d %s %s\n",pid,jobs[i].command_names[p],state_str);
        }
    }
}

int has_stopped_jobs(void){
    for(int i=0;i<job_count;i++){
        for(int p=0;p<jobs[i].process_count;p++){
            pid_t pid=jobs[i].pids[p];
            if(kill(pid,0)<0)continue;

            char path[64];
            snprintf(path,sizeof(path),"/proc/%d/stat",pid);
            FILE *f=fopen(path,"r");
            if(f){
                char buf[512];
                if(fgets(buf,sizeof(buf),f)){
                    char *paren=strrchr(buf,')');
                    if(paren && *(paren+1)==' '){
                        char state=*(paren+2);
                        if(state=='T' || state=='t'){
                            fclose(f);
                            return 1;
                        }
                    }
                }
                fclose(f);
            }
        }
    }
    return 0;
}

void kill_all_jobs(void){
    for(int i=0;i<job_count;i++){
        if(jobs[i].pgid>0){
            kill(-jobs[i].pgid,SIGHUP);
        }
    }
}


Job *get_job_by_number(int job_number) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_number == job_number) {
            return &jobs[i];
        }
    }
    return NULL;
}

int is_tracked_pid(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        for (int p = 0; p < jobs[i].process_count; p++) {
            if (jobs[i].pids[p] == pid) {
                return 1;
            }
        }
    }
    return 0;
}
