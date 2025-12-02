#include "memory.h"
#include "uart.h"

// Simple memory allocator
#define BLOCK_SIZE 64
#define MAX_BLOCKS 4096

struct mem_block {
    int free;
    unsigned long size;
    struct mem_block *next;
};

static unsigned long heap_start;
static unsigned long heap_end;
static struct mem_block *free_list;
static unsigned long total_allocated;
static unsigned long total_free;

void memory_init(unsigned long start, unsigned long end) {
    heap_start = start;
    heap_end = end;
    total_allocated = 0;
    total_free = end - start;
    
    // Align start address
    start = (start + 7) & ~7;
    
    // Initialize first free block
    free_list = (struct mem_block*)start;
    free_list->free = 1;
    free_list->size = end - start - sizeof(struct mem_block);
    free_list->next = NULL;
    
    // Debug output
    uart_puts("  Heap start: 0x");
    uart_put_hex(start);
    uart_puts("\n  Heap end: 0x");
    uart_put_hex(end);
    uart_puts("\n  Heap size: ");
    uart_put_dec(end - start);
    uart_puts(" bytes\n");
}

void* kmalloc(unsigned long size) {
    if (size == 0) return NULL;
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    struct mem_block *current = free_list;
    
    // First fit algorithm
    while (current != NULL) {
        if (current->free && current->size >= size) {
            // Found a suitable block
            current->free = 0;
            
            // Split block if there's enough space
            if (current->size > size + sizeof(struct mem_block) + 64) {
                struct mem_block *new_block = (struct mem_block*)((char*)current + sizeof(struct mem_block) + size);
                new_block->free = 1;
                new_block->size = current->size - size - sizeof(struct mem_block);
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            
            total_allocated += current->size;
            total_free -= current->size;
            
            return (void*)((char*)current + sizeof(struct mem_block));
        }
        current = current->next;
    }
    
    // Out of memory - print debug info
    uart_puts("  [KMALLOC] Out of memory! Requested: ");
    uart_put_dec(size);
    uart_puts(" bytes\n");
    
    return NULL; // Out of memory
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    struct mem_block *block = (struct mem_block*)((char*)ptr - sizeof(struct mem_block));
    block->free = 1;
    
    total_allocated -= block->size;
    total_free += block->size;
    
    // Coalesce with next block if it's free
    if (block->next && block->next->free) {
        block->size += sizeof(struct mem_block) + block->next->size;
        block->next = block->next->next;
    }
    
    // Coalesce with previous block if it's free
    struct mem_block *current = free_list;
    while (current && current->next != block) {
        current = current->next;
    }
    
    if (current && current->free) {
        current->size += sizeof(struct mem_block) + block->size;
        current->next = block->next;
    }
}

void* memset(void* dest, int val, unsigned long n) {
    unsigned char *d = dest;
    while (n--) {
        *d++ = (unsigned char)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned long n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void memory_stats(void) {
    uart_puts("Memory Statistics:\n");
    uart_puts("  Total: ");
    uart_put_dec(heap_end - heap_start);
    uart_puts(" bytes\n");
    uart_puts("  Allocated: ");
    uart_put_dec(total_allocated);
    uart_puts(" bytes\n");
    uart_puts("  Free: ");
    uart_put_dec(total_free);
    uart_puts(" bytes\n");
}