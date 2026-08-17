#include "bzlib.h"

#ifndef INPUT_BYTES
#define INPUT_BYTES 256
#endif

#define OUTPUT_BYTES (INPUT_BYTES + INPUT_BYTES / 100 + 601)

extern void heap_reset(void);
extern int runtime_has_failed(void);

static unsigned char input_data[INPUT_BYTES];
static unsigned char compressed_data[OUTPUT_BYTES];
static unsigned char restored_data[INPUT_BYTES];

static unsigned int fnv1a(const unsigned char *data, unsigned int length)
{
    unsigned int hash = 2166136261u;
    unsigned int i;

    for (i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static void prepare_input(void)
{
    unsigned int state = 0x31415927u;
    unsigned int i;

    for (i = 0; i < INPUT_BYTES; ++i) {
        state = state * 1664525u + 1013904223u;
        if ((i & 127u) < 80u)
            input_data[i] = (unsigned char)("branch-predictor-rob-compression"[i % 32u]);
        else if ((i & 31u) < 20u)
            input_data[i] = (unsigned char)(i >> 3);
        else
            input_data[i] = (unsigned char)(state >> 24);
    }
}

int benchmark_main(void)
{
    unsigned int compressed_len = OUTPUT_BYTES;
    unsigned int restored_len = INPUT_BYTES;
    unsigned int expected_hash;
    unsigned int i;
    int result;

    prepare_input();
    expected_hash = fnv1a(input_data, INPUT_BYTES);

    heap_reset();
    result = BZ2_bzBuffToBuffCompress(
        (char *)compressed_data,
        &compressed_len,
        (char *)input_data,
        INPUT_BYTES,
        1,
        0,
        30
    );
    if (result != BZ_OK || runtime_has_failed())
        return 1;

    heap_reset();
    result = BZ2_bzBuffToBuffDecompress(
        (char *)restored_data,
        &restored_len,
        (char *)compressed_data,
        compressed_len,
        0,
        0
    );
    if (result != BZ_OK || restored_len != INPUT_BYTES || runtime_has_failed())
        return 2;

    if (fnv1a(restored_data, restored_len) != expected_hash)
        return 3;

    for (i = 0; i < INPUT_BYTES; ++i)
        if (restored_data[i] != input_data[i])
            return 4;

    return 0;
}
