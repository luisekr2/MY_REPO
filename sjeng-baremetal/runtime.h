#ifndef SPEC_BAREMETAL_RUNTIME_H
#define SPEC_BAREMETAL_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    const char *name;
    const unsigned char *data;
    size_t size;
} spec_vfs_file;

extern const spec_vfs_file spec_vfs_files[];
extern const size_t spec_vfs_file_count;

FILE *spec_fopen(const char *name, const char *mode);
uint64_t spec_output_hash(void);
size_t spec_output_bytes(void);
int runtime_has_failed(void);

#endif
