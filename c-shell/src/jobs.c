#include "jobs.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_JOBS 64

static Job jobs[MAX_JOBS];
static int job_count=0;
static int next_job_number=1;

int add_job(pid_t pid){
    if (job_count>=MAX_JOBS)return -1;
    jobs[job_count].job_number=next_job_number++;
    jobs[job_count].pid=pid;
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

void reap_jobs(void){
    for(int i=0;i<job_count;i++){
        pid_t result=waitpid(jobs[i].pid,NULL,WNOHANG);
        if(result>0){
            remove_job(result);
            i--;
        }
    }
}