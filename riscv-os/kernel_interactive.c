#include "kernel.h"
#include "uart.h"
#include "process.h"
#include "filesystem.h"
#include "syscall.h"
#include "string.h"

// Shell process - INTERACTIVE command line
void shell_process(void) {
    char cmd_buffer[128];
    int cmd_pos;
    
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("  RISC-V OS Shell v1.0 (Interactive)\n");
    uart_puts("=====================================\n");
    uart_puts("Commands: help, ps, ls, cat <file>, mem, clear, exit\n");
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