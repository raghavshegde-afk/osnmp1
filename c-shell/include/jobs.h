#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct {
    int job_number;
    pid_t pid;
} Job;

int add_job(pid_t pid);
void remove_job(pid_t pid);
void print_jobs();
void reap_jobs();

#endif