#include "kernel.h"
#include "uart.h"
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"
#include "filesystem.h"
#include "string.h"

// External symbols from linker script
extern char _heap_start[];
extern char _heap_end[];

void kernel_main(void) {
    // Initialize UART for output
    uart_init();
    uart_puts("\n=== RISC-V OS Booting ===\n");
    uart_puts("Kernel Version 1.0\n\n");
    
    // Initialize memory management
    uart_puts("Initializing memory management...\n");
    memory_init((unsigned long)_heap_start, (unsigned long)_heap_end);
    
    // Initialize process management
    uart_puts("Initializing process management...\n");
    process_init();
    
    // Initialize filesystem
    uart_puts("Initializing filesystem...\n");
    fs_init();
    
    // Create initial processes
    uart_puts("Creating initial processes...\n");
    
    // Create init process (PID 1)
    int init_pid = create_process("init", init_process, 1);
    uart_puts("Created init process (PID ");
    uart_put_hex(init_pid);
    uart_puts(")\n");
    
    // Create shell process
    int shell_pid = create_process("shell", shell_process, 1);
    uart_puts("Created shell process (PID ");
    uart_put_hex(shell_pid);
    uart_puts(")\n");
    
    // Create idle process (lowest priority)
    int idle_pid = create_process("idle", idle_process, 0);
    uart_puts("Created idle process (PID ");
    uart_put_hex(idle_pid);
    uart_puts(")\n");
    
    uart_puts("\n=== Boot Complete ===\n");
    uart_puts("Starting scheduler...\n\n");
    
    // Enable interrupts and start scheduler
    enable_interrupts();
    scheduler_start();
    
    // Should never reach here
    uart_puts("ERROR: Scheduler returned!\n");
    while(1) {
        asm volatile("wfi");
    }
}

// Trap handler called from assembly
void trap_handler(struct trap_frame *frame) {
    unsigned long cause = read_csr(mcause);
    unsigned long epc = read_csr(mepc);
    
    // Check if it's an interrupt or exception
    if (cause & (1UL << 63)) {
        // Interrupt
        cause &= ~(1UL << 63);
        
        switch(cause) {
            case 7:  // Machine timer interrupt
                handle_timer_interrupt();
                break;
            case 11: // Machine external interrupt
                handle_external_interrupt();
                break;
            default:
                uart_puts("Unknown interrupt: ");
                uart_put_hex(cause);
                uart_puts("\n");
                break;
        }
    } else {
        // Exception
        switch(cause) {
            case 8:  // Environment call from U-mode
            case 9:  // Environment call from S-mode
            case 11: // Environment call from M-mode
                handle_syscall(frame);
                write_csr(mepc, epc + 4); // Move past ecall instruction
                break;
            default:
                uart_puts("EXCEPTION: ");
                uart_put_hex(cause);
                uart_puts(" at PC: ");
                uart_put_hex(epc);
                uart_puts("\n");
                // Kill current process
                process_exit(-1);
                break;
        }
    }
}

// Timer interrupt handler for preemptive scheduling
void handle_timer_interrupt(void) {
    // Set next timer interrupt
    set_timer(10000000); // 100ms at 100MHz
    
    // Trigger context switch
    schedule();
}

void handle_external_interrupt(void) {
    // Handle external interrupts (e.g., keyboard, disk)
    uart_puts("External interrupt\n");
}

void enable_interrupts(void) {
    // Set timer
    set_timer(10000000);
    
    // Enable interrupts in mstatus
    unsigned long mstatus = read_csr(mstatus);
    mstatus |= (1 << 3); // MIE bit
    write_csr(mstatus, mstatus);
}

void set_timer(unsigned long cycles) {
    // QEMU virt machine memory-mapped timer addresses
    volatile uint64_t *mtime = (uint64_t*)0x200bff8;
    volatile uint64_t *mtimecmp = (uint64_t*)0x2004000;
    
    *mtimecmp = *mtime + cycles;
}

// Init process - first user process
void init_process(void) {
    uart_puts("[INIT] Init process starting\n");
    
    // Create some demo files
    fs_create_file("hello.txt", "Hello from the filesystem!\n", 28);
    fs_create_file("readme.txt", "RISC-V OS - A simple operating system\n", 39);
    
    // Create a demo program
    fs_create_file("test.bin", "\x93\x08\x50\x00", 4); // li a7, 5 (exit syscall)
    
    uart_puts("[INIT] Initialization complete\n");
    
    // Sleep forever
    while(1) {
        syscall_sleep(1000);
    }
}

// Shell process - interactive command line
void shell_process(void) {
    char cmd_buffer[128];
    
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("  RISC-V OS Shell v1.0\n");
    uart_puts("=====================================\n");
    uart_puts("Commands: help, ps, ls, cat, mem, exit\n");
    uart_puts("\n");
    
    // Demo: execute some commands automatically
    char *demo_cmds[] = {"help", "ps", "ls", "cat readme.txt", "mem", "exit"};
    int num_cmds = 6;
    
    for (int demo_step = 0; demo_step < num_cmds; demo_step++) {
        uart_puts("$ ");
        strcpy(cmd_buffer, demo_cmds[demo_step]);
        uart_puts(cmd_buffer);
        uart_puts("\n");
        
        // Parse and execute command
        if (strcmp(cmd_buffer, "help") == 0) {
            uart_puts("Available commands:\n");
            uart_puts("  help  - Show this help\n");
            uart_puts("  ps    - List processes\n");
            uart_puts("  ls    - List files\n");
            uart_puts("  cat   - Display file contents\n");
            uart_puts("  exec  - Execute a program\n");
            uart_puts("  mem   - Show memory usage\n");
            uart_puts("  exit  - Exit shell\n");
        }
        else if (strcmp(cmd_buffer, "ps") == 0) {
            process_list();
        }
        else if (strcmp(cmd_buffer, "ls") == 0) {
            fs_list_files();
        }
        else if (strncmp(cmd_buffer, "cat ", 4) == 0) {
            char *filename = cmd_buffer + 4;
            struct file *f = fs_open(filename);
            if (f) {
                uart_puts(f->data);
                if (f->data[f->size - 1] != '\n') {
                    uart_puts("\n");
                }
            } else {
                uart_puts("File not found: ");
                uart_puts(filename);
                uart_puts("\n");
            }
        }
        else if (strcmp(cmd_buffer, "mem") == 0) {
            memory_stats();
        }
        else if (strcmp(cmd_buffer, "exit") == 0) {
            uart_puts("Exiting shell...\n");
            syscall_exit(0);
            break;
        }
        else if (cmd_buffer[0] != '\0') {
            uart_puts("Unknown command: ");
            uart_puts(cmd_buffer);
            uart_puts("\n");
        }
        
        uart_puts("\n");
    }
    
    uart_puts("\n=== Shell Demo Complete ===\n");
    uart_puts("OS is still running. Press Ctrl-A then X to exit QEMU.\n\n");
    
    // Keep running
    while(1) {
        syscall_sleep(10000);
    }
}

// Idle process - runs when nothing else can run
void idle_process(void) {
    while(1) {
        asm volatile("wfi"); // Wait for interrupt
    }
}