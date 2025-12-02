#include "filesystem.h"
#include "memory.h"
#include "uart.h"
#include "string.h"

#define MAX_FILES 64

static struct file files[MAX_FILES];
static int num_files = 0;

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].name[0] = '\0';
        files[i].data = NULL;
        files[i].size = 0;
    }
    num_files = 0;
}

int fs_create_file(const char *name, const char *data, unsigned long size) {
    if (num_files >= MAX_FILES) {
        uart_puts("ERROR: Filesystem full\n");
        return -1;
    }
    
    // Check if file already exists
    for (int i = 0; i < num_files; i++) {
        if (strcmp(files[i].name, name) == 0) {
            uart_puts("ERROR: File already exists: ");
            uart_puts(name);
            uart_puts("\n");
            return -1;
        }
    }
    
    struct file *f = &files[num_files];
    strncpy(f->name, name, 63);
    f->name[63] = '\0';
    f->size = size;
    
    // Allocate memory for file data
    f->data = (char*)kmalloc(size + 1);
    if (!f->data) {
        uart_puts("ERROR: Failed to allocate memory for file\n");
        return -1;
    }
    
    memcpy(f->data, data, size);
    f->data[size] = '\0'; // Null terminate for convenience
    
    num_files++;
    return 0;
}

struct file* fs_open(const char *name) {
    for (int i = 0; i < num_files; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

void fs_list_files(void) {
    uart_puts("Files:\n");
    uart_puts("  NAME                SIZE\n");
    uart_puts("  ------------------- -----\n");
    
    if (num_files == 0) {
        uart_puts("  (no files)\n");
        return;
    }
    
    for (int i = 0; i < num_files; i++) {
        uart_puts("  ");
        uart_puts(files[i].name);
        
        // Pad to 20 characters
        int len = strlen(files[i].name);
        for (int j = len; j < 20; j++) {
            uart_putc(' ');
        }
        
        uart_put_dec(files[i].size);
        uart_puts("\n");
    }
}

int fs_delete_file(const char *name) {
    for (int i = 0; i < num_files; i++) {
        if (strcmp(files[i].name, name) == 0) {
            // Free data
            if (files[i].data) {
                kfree(files[i].data);
            }
            
            // Shift remaining files down
            for (int j = i; j < num_files - 1; j++) {
                files[j] = files[j + 1];
            }
            
            num_files--;
            return 0;
        }
    }
    return -1;
}