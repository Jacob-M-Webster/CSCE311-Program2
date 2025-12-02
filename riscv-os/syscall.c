#include "syscall.h"
#include "process.h"
#include "uart.h"
#include "filesystem.h"
#include "string.h"

// System call numbers
#define SYS_EXIT    1
#define SYS_WRITE   2
#define SYS_READ    3
#define SYS_SLEEP   4
#define SYS_GETPID  5
#define SYS_EXEC    6
#define SYS_OPEN    7
#define SYS_CLOSE   8

void handle_syscall(struct trap_frame *frame) {
    unsigned long syscall_num = frame->regs[17]; // a7 register
    unsigned long arg0 = frame->regs[10]; // a0
    unsigned long arg1 = frame->regs[11]; // a1
    unsigned long arg2 = frame->regs[12]; // a2
    unsigned long result = 0;
    
    switch (syscall_num) {
        case SYS_EXIT:
            process_exit((int)arg0);
            break;
            
        case SYS_WRITE: {
            // arg0 = fd, arg1 = buffer, arg2 = length
            if (arg0 == 1) { // stdout
                const char *buf = (const char*)arg1;
                for (unsigned long i = 0; i < arg2; i++) {
                    uart_putc(buf[i]);
                }
                result = arg2;
            }
            break;
        }
        
        case SYS_READ: {
            // arg0 = fd, arg1 = buffer, arg2 = length
            if (arg0 == 0) { // stdin
                char *buf = (char*)arg1;
                for (unsigned long i = 0; i < arg2; i++) {
                    buf[i] = uart_getc();
                    if (buf[i] == '\n') {
                        result = i + 1;
                        break;
                    }
                }
            }
            break;
        }
        
        case SYS_SLEEP:
            syscall_sleep(arg0);
            break;
            
        case SYS_GETPID: {
            struct process *proc = get_current_process();
            result = proc ? proc->pid : 0;
            break;
        }
        
        case SYS_EXEC: {
            const char *filename = (const char*)arg0;
            result = syscall_exec(filename);
            break;
        }
        
        case SYS_OPEN: {
            const char *filename = (const char*)arg0;
            struct file *f = fs_open(filename);
            result = f ? (unsigned long)f : 0;
            break;
        }
        
        case SYS_CLOSE:
            // Not implemented in this simple version
            result = 0;
            break;
            
        default:
            uart_puts("Unknown syscall: ");
            uart_put_hex(syscall_num);
            uart_puts("\n");
            result = -1;
            break;
    }
    
    // Return result in a0
    frame->regs[10] = result;
}

void syscall_exit(int status) {
    asm volatile(
        "li a7, 1\n"
        "mv a0, %0\n"
        "ecall\n"
        :: "r"(status)
    );
}

int syscall_write(int fd, const char *buf, unsigned long len) {
    int result;
    asm volatile(
        "li a7, 2\n"
        "mv a0, %1\n"
        "mv a1, %2\n"
        "mv a2, %3\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
        : "r"(fd), "r"(buf), "r"(len)
    );
    return result;
}

int syscall_read(int fd, char *buf, unsigned long len) {
    int result;
    asm volatile(
        "li a7, 3\n"
        "mv a0, %1\n"
        "mv a1, %2\n"
        "mv a2, %3\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
        : "r"(fd), "r"(buf), "r"(len)
    );
    return result;
}

void syscall_sleep(unsigned long ms) {
    // Convert ms to timer ticks (assuming 100MHz clock)
    unsigned long ticks = ms * 100000;
    process_sleep(ticks);
    schedule();
}

int syscall_getpid(void) {
    int result;
    asm volatile(
        "li a7, 5\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(result)
    );
    return result;
}

int syscall_exec(const char *filename) {
    // Load and execute a program from filesystem
    struct file *f = fs_open(filename);
    if (!f) {
        uart_puts("Cannot open: ");
        uart_puts(filename);
        uart_puts("\n");
        return -1;
    }
    
    // In a real OS, we would:
    // 1. Parse the binary format (ELF)
    // 2. Allocate memory for code and data
    // 3. Load sections into memory
    // 4. Set up initial stack
    // 5. Jump to entry point
    
    // For this demo, we just acknowledge the request
    uart_puts("Executing: ");
    uart_puts(filename);
    uart_puts(" (");
    uart_put_dec(f->size);
    uart_puts(" bytes)\n");
    
    // Simulate execution
    uart_puts("Program executed successfully\n");
    
    return 0;
}