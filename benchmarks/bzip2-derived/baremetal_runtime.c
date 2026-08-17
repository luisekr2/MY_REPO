#include <stdlib.h>

#define HEAP_BYTES (512u * 1024u)

static unsigned char heap[HEAP_BYTES] __attribute__((aligned(64)));
static size_t heap_offset;
static int runtime_failed;

void heap_reset(void)
{
    heap_offset = 0;
}

int runtime_has_failed(void)
{
    return runtime_failed;
}

void *malloc(size_t size)
{
    size_t aligned = (size + 63u) & ~(size_t)63u;
    size_t old = heap_offset;

    if (aligned > HEAP_BYTES - old) {
        runtime_failed = 1;
        return NULL;
    }
    heap_offset = old + aligned;
    return &heap[old];
}

void free(void *ptr)
{
    (void)ptr;
}

void exit(int status)
{
    runtime_failed = status ? status : 1;
}

void bz_internal_error(int errcode)
{
    runtime_failed = errcode ? errcode : 1;
}
