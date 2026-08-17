#include "runtime.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern unsigned char __heap_start[];
extern unsigned char __heap_end[];

static unsigned char *heap_cursor = __heap_start;
static uint64_t output_hash = UINT64_C(1469598103934665603);
static size_t output_bytes;
static int failed;
static void hash_bytes(const unsigned char *data, size_t size);

static int console_put(char value, FILE *stream)
{
    (void)stream;
    hash_bytes((const unsigned char *)&value, 1);
    return (unsigned char)value;
}

static int console_get(FILE *stream) { (void)stream; return _FDEV_EOF; }
static int console_flush(FILE *stream) { (void)stream; return 0; }
static FILE console_in = FDEV_SETUP_STREAM(NULL, console_get, console_flush,
                                           _FDEV_SETUP_READ);
static FILE console_out = FDEV_SETUP_STREAM(console_put, NULL, console_flush,
                                            _FDEV_SETUP_WRITE);
FILE *const stdin = &console_in;
FILE *const stdout = &console_out;
FILE *const stderr = &console_out;

static void hash_bytes(const unsigned char *data, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        output_hash ^= data[i];
        output_hash *= UINT64_C(1099511628211);
    }
    output_bytes += size;
}

static ssize_t sink_write(void *cookie, const void *buffer, size_t length)
{
    (void)cookie;
    if (length > 0)
        hash_bytes((const unsigned char *)buffer, length);
    return (ssize_t)length;
}

FILE *spec_fopen(const char *name, const char *mode)
{
    size_t i;
    if (mode && mode[0] == 'r') {
        for (i = 0; i < spec_vfs_file_count; i++) {
            if (strcmp(name, spec_vfs_files[i].name) == 0)
                return fmemopen((void *)spec_vfs_files[i].data,
                                spec_vfs_files[i].size ? spec_vfs_files[i].size :
                                strlen((const char *)spec_vfs_files[i].data), "r");
        }
        errno = ENOENT;
        return NULL;
    }
    return funopen(NULL, NULL, sink_write, NULL, NULL);
}

void *_sbrk(ptrdiff_t increment)
{
    unsigned char *old = heap_cursor;
    if (increment < 0 || (size_t)increment > (size_t)(__heap_end - heap_cursor)) {
        errno = ENOMEM;
        failed = 1;
        return (void *)-1;
    }
    heap_cursor += increment;
    return old;
}

ssize_t _write(int fd, const void *buffer, size_t size)
{
    (void)fd;
    hash_bytes((const unsigned char *)buffer, size);
    return (ssize_t)size;
}

ssize_t _read(int fd, void *buffer, size_t size)
{
    (void)fd; (void)buffer; (void)size;
    return 0;
}

int _close(int fd) { (void)fd; return 0; }
off_t _lseek(int fd, off_t offset, int whence)
{
    (void)fd; (void)offset; (void)whence; return 0;
}
int _fstat(int fd, struct stat *status)
{
    (void)fd;
    status->st_mode = S_IFCHR;
    return 0;
}
int _isatty(int fd) { (void)fd; return 1; }
int _kill(int pid, int signal) { (void)pid; (void)signal; return -1; }
int _getpid(void) { return 1; }

uint64_t spec_output_hash(void) { return output_hash; }
size_t spec_output_bytes(void) { return output_bytes; }
int runtime_has_failed(void) { return failed; }

void _exit(int status)
{
    register long code asm("a0") = status;
    asm volatile(".word 0x0000006b" : : "r"(code));
    for (;;) {}
}
