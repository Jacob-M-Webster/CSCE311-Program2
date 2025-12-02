#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "kernel.h"

struct file {
    char name[64];
    char *data;
    unsigned long size;
};

void fs_init(void);
int fs_create_file(const char *name, const char *data, unsigned long size);
struct file* fs_open(const char *name);
void fs_list_files(void);
int fs_delete_file(const char *name);

#endif