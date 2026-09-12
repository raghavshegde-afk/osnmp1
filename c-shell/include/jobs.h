#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_PIPELINE_PROCS 64

typedef struct {
    int job_number;
    pid_t pgid;
    char job_name[256];
    int process_count;
    pid_t pids[MAX_PIPELINE_PROCS];
    char command_names[MAX_PIPELINE_PROCS][256];
} Job;

int add_job(pid_t pid,char *command_name);
int add_job_group(pid_t pgid, pid_t *pids, char **names, int count);
void remove_job(pid_t pid);
void print_jobs();
void record_child_exit(pid_t pid, int status);
void reap_jobs();
void print_activities();
int has_stopped_jobs(void);
void kill_all_jobs(void);
Job *get_job_by_number(int job_number);
int is_tracked_pid(pid_t pid);

#endif
