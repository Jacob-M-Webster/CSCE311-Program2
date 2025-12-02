#include "process.h"
#include "memory.h"
#include "uart.h"
#include "string.h"

#define MAX_PROCESSES 32
#define STACK_SIZE 8192

static struct process processes[MAX_PROCESSES];
static int next_pid = 1;
static struct process *current_process = NULL;

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = PROC_UNUSED;
    }
}

int create_process(const char *name, void (*entry)(void), int priority) {
    // Find free process slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        uart_puts("ERROR: No free process slots\n");
        return -1;
    }
    
    struct process *proc = &processes[slot];
    
    // Initialize process
    proc->pid = next_pid++;
    strncpy(proc->name, name, 31);
    proc->name[31] = '\0';
    proc->state = PROC_READY;
    proc->priority = priority;
    proc->entry = entry;
    proc->sleep_until = 0;
    
    // Allocate stack
    proc->stack = (unsigned long)kmalloc(STACK_SIZE);
    if (proc->stack == 0) {
        uart_puts("ERROR: Failed to allocate stack\n");
        proc->state = PROC_UNUSED;
        return -1;
    }
    
    // Initialize stack pointer to top of stack
    proc->sp = proc->stack + STACK_SIZE;
    
    // Set up initial context
    // Push entry point and initial registers onto stack
    proc->sp -= 8;
    *(unsigned long*)proc->sp = (unsigned long)entry; // PC
    proc->sp -= 8;
    *(unsigned long*)proc->sp = 0; // RA
    
    // Reserve space for saved registers
    proc->sp -= 30 * 8; // s0-s11, t0-t6, a0-a7
    
    return proc->pid;
}

struct process* get_current_process(void) {
    return current_process;
}

void set_current_process(struct process *proc) {
    current_process = proc;
}

struct process* get_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != PROC_UNUSED) {
            return &processes[i];
        }
    }
    return NULL;
}

void process_list(void) {
    uart_puts("PID   STATE      PRIORITY  NAME\n");
    uart_puts("----  ---------  --------  ----\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROC_UNUSED) {
            uart_put_dec(processes[i].pid);
            uart_puts("     ");
            
            switch (processes[i].state) {
                case PROC_READY: uart_puts("READY    "); break;
                case PROC_RUNNING: uart_puts("RUNNING  "); break;
                case PROC_SLEEPING: uart_puts("SLEEPING "); break;
                case PROC_WAITING: uart_puts("WAITING  "); break;
                case PROC_ZOMBIE: uart_puts("ZOMBIE   "); break;
                default: uart_puts("UNKNOWN  "); break;
            }
            
            uart_puts("  ");
            uart_put_dec(processes[i].priority);
            uart_puts("         ");
            uart_puts(processes[i].name);
            uart_puts("\n");
        }
    }
}

void process_sleep(unsigned long ticks) {
    if (current_process) {
        current_process->state = PROC_SLEEPING;
        // Memory-mapped mtime register
        volatile uint64_t *mtime = (uint64_t*)0x200bff8;
        current_process->sleep_until = *mtime + ticks;
    }
}

void process_wake_sleeping(void) {
    // Memory-mapped mtime register
    volatile uint64_t *mtime = (uint64_t*)0x200bff8;
    unsigned long current_time = *mtime;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_SLEEPING) {
            if (current_time >= processes[i].sleep_until) {
                processes[i].state = PROC_READY;
            }
        }
    }
}

void process_exit(int status) {
    if (current_process) {
        uart_puts("[KERNEL] Process ");
        uart_put_dec(current_process->pid);
        uart_puts(" (");
        uart_puts(current_process->name);
        uart_puts(") exited with status ");
        uart_put_dec(status);
        uart_puts("\n");
        
        current_process->state = PROC_ZOMBIE;
        current_process->exit_status = status;
        
        // Clean up resources
        if (current_process->stack) {
            kfree((void*)current_process->stack);
            current_process->stack = 0;
        }
        
        // Schedule next process
        schedule();
    }
}

struct process* get_next_process(void) {
    // Simple round-robin with priority
    struct process *best = NULL;
    int best_priority = -1;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_READY) {
            if (processes[i].priority > best_priority) {
                best = &processes[i];
                best_priority = processes[i].priority;
            }
        }
    }
    
    return best;
}