#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#ifndef HEAP_BYTES
#define HEAP_BYTES (256u * 1024u * 1024u)
#endif

typedef struct bare_file {
    const unsigned char *cursor;
    const unsigned char *end;
    int sink;
} FILE;

extern const unsigned char spec_input_start[];
extern const unsigned char spec_input_end[];

static unsigned char heap[HEAP_BYTES] __attribute__((aligned(64)));
static size_t heap_offset;
static int failed;
static FILE input_file;
static FILE sink_file;
FILE *stdout = &sink_file;
FILE *stderr = &sink_file;

typedef struct allocation_header {
    size_t size;
} allocation_header;

static void *copy_bytes(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memset(void *dst, int value, size_t n)
{
    unsigned char *d = dst;
    while (n--)
        *d++ = (unsigned char)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    return copy_bytes(dst, src, n);
}

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++)) {}
    return ret;
}

void *malloc(size_t size)
{
    size_t total = (sizeof(allocation_header) + size + 63u) & ~(size_t)63u;
    allocation_header *h;
    if (total > HEAP_BYTES - heap_offset) {
        failed = 1;
        return 0;
    }
    h = (allocation_header *)&heap[heap_offset];
    heap_offset += total;
    h->size = size;
    return h + 1;
}

void *calloc(size_t count, size_t size)
{
    size_t total;
    void *p;
    if (count && size > (size_t)-1 / count)
        return 0;
    total = count * size;
    p = malloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

void *realloc(void *old, size_t size)
{
    allocation_header *h;
    void *p;
    size_t copy;
    if (!old)
        return malloc(size);
    h = (allocation_header *)old - 1;
    p = malloc(size);
    if (!p)
        return 0;
    copy = h->size < size ? h->size : size;
    copy_bytes(p, old, copy);
    return p;
}

void free(void *ptr) { (void)ptr; }
int runtime_has_failed(void) { return failed; }
void exit(int status) { failed = status ? status : 1; }

FILE *fopen(const char *name, const char *mode)
{
    (void)name;
    if (mode[0] == 'r') {
        input_file.cursor = spec_input_start;
        input_file.end = spec_input_end;
        input_file.sink = 0;
        return &input_file;
    }
    sink_file.cursor = sink_file.end = 0;
    sink_file.sink = 1;
    return &sink_file;
}

char *fgets(char *dst, int size, FILE *file)
{
    int i = 0;
    if (!file || file->sink || file->cursor >= file->end || size <= 1)
        return 0;
    while (i + 1 < size && file->cursor < file->end) {
        char c = (char)*file->cursor++;
        dst[i++] = c;
        if (c == '\n')
            break;
    }
    dst[i] = 0;
    return dst;
}

static int parse_long(const char **cursor, long *value)
{
    const char *p = *cursor;
    long sign = 1;
    long v = 0;
    int digits = 0;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '-') { sign = -1; p++; }
    else if (*p == '+') p++;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p++ - '0');
        digits++;
    }
    if (!digits)
        return 0;
    *value = v * sign;
    *cursor = p;
    return 1;
}

int sscanf(const char *str, const char *format, ...)
{
    va_list ap;
    int converted = 0;
    const char *p = str;
    const char *f = format;
    va_start(ap, format);
    while (*f) {
        if (f[0] == '%' && f[1] == 'l' && f[2] == 'd') {
            long *out = va_arg(ap, long *);
            if (!parse_long(&p, out)) break;
            converted++;
            f += 3;
        } else {
            f++;
        }
    }
    va_end(ap);
    return converted;
}

int fclose(FILE *file) { (void)file; return 0; }
int printf(const char *format, ...) { (void)format; return 0; }
int fprintf(FILE *file, const char *format, ...) { (void)file; (void)format; return 0; }
int fflush(FILE *file) { (void)file; return 0; }
void __assert_fail(const char *a, const char *f, unsigned int l, const char *fn)
{
    (void)a; (void)f; (void)l; (void)fn; failed = 1;
}
