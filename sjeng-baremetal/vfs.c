#include "runtime.h"
extern const unsigned char sjeng_input_start[], sjeng_input_end[];
const spec_vfs_file spec_vfs_files[] = {
    { "test-mini.txt", sjeng_input_start, 0 }
};
const size_t spec_vfs_file_count = 1;
