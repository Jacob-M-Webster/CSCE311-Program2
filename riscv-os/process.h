#ifndef PROCESS_H
#define PROCESS_H

#include "kernel.h"

enum proc_state {
    PROC_UNUSED,
    PROC_READY,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_WAITING,
    PROC_ZOMBIE
};

struct process {
    int pid;
    char name[32];
    enum proc_state state;
    int priority;
    unsigned long sp;           // Stack pointer
    unsigned long stack;        // Stack base
    void (*entry)(void);        // Entry point
    unsigned long sleep_until;  // Wake time for sleeping processes
    int exit_status;
};

void process_init(void);
int create_process(const char *name, void (*entry)(void), int priority);
struct process* get_current_process(void);
void set_current_process(struct process *proc);
struct process* get_process_by_pid(int pid);
void process_list(void);
void process_sleep(unsigned long ticks);
void process_wake_sleeping(void);
void process_exit(int status);
struct process* get_next_process(void);

#endif