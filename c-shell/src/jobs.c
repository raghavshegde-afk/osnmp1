#include "jobs.h"
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_JOBS 64
#define MAX_JOB_EVENTS 128

static Job jobs[MAX_JOBS];
static int job_count=0;
static int next_job_number=1;
static volatile sig_atomic_t event_pids[MAX_JOB_EVENTS];
static volatile sig_atomic_t event_statuses[MAX_JOB_EVENTS];
static volatile sig_atomic_t event_head=0;
static volatile sig_atomic_t event_tail=0;

// int add_job(pid_t pid){
int add_job(pid_t pid,char *command_name){
    if (job_count>=MAX_JOBS)return -1;
    jobs[job_count].job_number=next_job_number++;
    jobs[job_count].pid=pid;
    snprintf(jobs[job_count].command_name,sizeof(jobs[job_count].command_name),"%s",command_name);
    int number=jobs[job_count].job_number;
    job_count++;
    return number;
}

void remove_job(pid_t pid){
    for (int i=0;i<job_count;i++) {
        if (jobs[i].pid==pid) {
            for (int j=i;j<job_count-1;j++)jobs[j]=jobs[j+1];
            job_count--;
            return;
        }
    }
}

void print_jobs(){
    for (int i=0;i<job_count;i++)printf("[%d] %d\n",jobs[i].job_number,jobs[i].pid);
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
            if(jobs[i].pid==pid){
                if(WIFEXITED(status) && WEXITSTATUS(status)==0)
                    printf("%s with pid %d exited normally\n",jobs[i].command_name,pid);
                else
                    printf("%s with pid %d exited abnormally\n",jobs[i].command_name,pid);
                remove_job(pid);
                break;
            }
        }
    }

    sigprocmask(SIG_SETMASK,&oldset,NULL);
}
