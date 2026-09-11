#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct {
    int job_number;
    pid_t pid;
    char command_name[256];
} Job;

// int add_job(pid_t pid);
int add_job(pid_t pid,char *command_name);
void remove_job(pid_t pid);
void print_jobs();
void record_child_exit(pid_t pid, int status);
void reap_jobs();

#endif
