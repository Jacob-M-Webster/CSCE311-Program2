#ifndef MEMORY_H
#define MEMORY_H

#include "kernel.h"

void memory_init(unsigned long start, unsigned long end);
void* kmalloc(unsigned long size);
void kfree(void* ptr);
void* memset(void* dest, int val, unsigned long n);
void* memcpy(void* dest, const void* src, unsigned long n);
void memory_stats(void);

#endif