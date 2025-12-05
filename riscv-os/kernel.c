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

// Shell process - INTERACTIVE command line
void shell_process(void) {
    char cmd_buffer[128];
    int cmd_pos;
    
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("  RISC-V OS Shell v1.0 (Interactive)\n");
    uart_puts("=====================================\n");
    uart_puts("Type 'help' for command list\n");
    uart_puts("\n");
    
    while(1) {
        uart_puts("$ ");
        
        // Read command from UART
        cmd_pos = 0;
        while(1) {
            char c = uart_getc();
            
            // Handle backspace
            if (c == 127 || c == 8) {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    uart_puts("\b \b");  // Erase character on screen
                }
                continue;
            }
            
            // Handle enter
            if (c == '\r' || c == '\n') {
                cmd_buffer[cmd_pos] = '\0';
                uart_puts("\n");
                break;
            }
            
            // Handle normal characters
            if (c >= 32 && c < 127 && cmd_pos < 127) {
                cmd_buffer[cmd_pos++] = c;
                uart_putc(c);  // Echo character
            }
        }
        
        // Skip empty commands
        if (cmd_pos == 0) {
            continue;
        }
        
        // Parse and execute command
        if (strcmp(cmd_buffer, "help") == 0) {
            uart_puts("\nAvailable commands:\n");
            uart_puts("  help           - Show this help\n");
            uart_puts("  ps             - List processes\n");
            uart_puts("  ls             - List files\n");
            uart_puts("  cat <file>     - Display file contents\n");
            uart_puts("  echo <text>    - Print text to screen\n");
            uart_puts("  echo <text> > <file> - Write text to file\n");
            uart_puts("  create <file>  - Create a test file\n");
            uart_puts("  exec <file>    - Execute a program\n");
            uart_puts("  mem            - Show memory usage\n");
            uart_puts("  clear          - Clear screen\n");
            uart_puts("  exit           - Exit shell\n\n");
        }
        else if (strcmp(cmd_buffer, "ps") == 0) {
            uart_puts("\n");
            process_list();
            uart_puts("\n");
        }
        else if (strcmp(cmd_buffer, "ls") == 0) {
            uart_puts("\n");
            fs_list_files();
            uart_puts("\n");
        }
        else if (strncmp(cmd_buffer, "cat ", 4) == 0) {
            char *filename = cmd_buffer + 4;
            // Trim leading spaces
            while (*filename == ' ') filename++;
            
            if (*filename == '\0') {
                uart_puts("Usage: cat <filename>\n\n");
            } else {
                struct file *f = fs_open(filename);
                if (f) {
                    uart_puts("\n");
                    uart_puts(f->data);
                    if (f->size > 0 && f->data[f->size - 1] != '\n') {
                        uart_puts("\n");
                    }
                    uart_puts("\n");
                } else {
                    uart_puts("File not found: ");
                    uart_puts(filename);
                    uart_puts("\n\n");
                }
            }
        }
        else if (strncmp(cmd_buffer, "create ", 7) == 0) {
            char *filename = cmd_buffer + 7;
            while (*filename == ' ') filename++;
            
            if (*filename == '\0') {
                uart_puts("Usage: create <filename>\n\n");
            } else {
                char content[128];
                strcpy(content, "This is a test file created at runtime: ");
                strcat(content, filename);
                strcat(content, "\n");
                
                if (fs_create_file(filename, content, strlen(content)) == 0) {
                    uart_puts("File created: ");
                    uart_puts(filename);
                    uart_puts("\n\n");
                } else {
                    uart_puts("Failed to create file\n\n");
                }
            }
        }
        else if (strncmp(cmd_buffer, "echo ", 5) == 0) {
            char *text = cmd_buffer + 5;
            while (*text == ' ') text++;
            
            if (*text == '\0') {
                uart_puts("\n");
                return;
            }
            
            // Check for redirection: echo text > filename
            char *redirect = text;
            char *filename = NULL;
            
            // Find the '>' character
            while (*redirect != '\0') {
                if (*redirect == '>') {
                    *redirect = '\0';  // Terminate text here
                    filename = redirect + 1;
                    while (*filename == ' ') filename++;  // Skip spaces
                    break;
                }
                redirect++;
            }
            
            if (filename && *filename != '\0') {
                // Write to file
                // Remove trailing spaces from text
                char *end = redirect - 1;
                while (end > text && (*end == ' ' || *end == '\0')) {
                    *end = '\0';
                    end--;
                }
                
                // Add newline to text
                char file_content[256];
                strcpy(file_content, text);
                strcat(file_content, "\n");
                
                if (fs_create_file(filename, file_content, strlen(file_content)) == 0) {
                    uart_puts("Written to ");
                    uart_puts(filename);
                    uart_puts("\n\n");
                } else {
                    uart_puts("Failed to write to file\n\n");
                }
            } else {
                // Just print to screen
                uart_puts("\n");
                uart_puts(text);
                uart_puts("\n\n");
            }
        }
        else if (strncmp(cmd_buffer, "exec ", 5) == 0) {
            char *filename = cmd_buffer + 5;
            while (*filename == ' ') filename++;
            
            if (*filename == '\0') {
                uart_puts("Usage: exec <filename>\n\n");
            } else {
                syscall_exec(filename);
                uart_puts("\n");
            }
        }
        else if (strcmp(cmd_buffer, "mem") == 0) {
            uart_puts("\n");
            memory_stats();
            uart_puts("\n");
        }
        else if (strcmp(cmd_buffer, "clear") == 0) {
            // ANSI escape code to clear screen
            uart_puts("\033[2J\033[H");
            uart_puts("=====================================\n");
            uart_puts("  RISC-V OS Shell v1.0 (Interactive)\n");
            uart_puts("=====================================\n\n");
        }
        else if (strcmp(cmd_buffer, "exit") == 0) {
            uart_puts("\nExiting shell...\n");
            uart_puts("Press Ctrl-A then X to exit QEMU\n\n");
            syscall_exit(0);
            break;
        }
        else {
            uart_puts("Unknown command: ");
            uart_puts(cmd_buffer);
            uart_puts("\n");
            uart_puts("Type 'help' for available commands\n\n");
        }
    }
    
    // Keep running after exit
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