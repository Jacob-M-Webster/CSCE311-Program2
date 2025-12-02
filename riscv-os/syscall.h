#ifndef SYSCALL_H
#define SYSCALL_H

#include "kernel.h"

void handle_syscall(struct trap_frame *frame);
void syscall_exit(int status);
int syscall_write(int fd, const char *buf, unsigned long len);
int syscall_read(int fd, char *buf, unsigned long len);
void syscall_sleep(unsigned long ms);
int syscall_getpid(void);
int syscall_exec(const char *filename);

// Forward declaration
void schedule(void);

#endif