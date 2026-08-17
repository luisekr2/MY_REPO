#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define HEAP_BYTES (32u * 1024u * 1024u)
#define PI 3.14159265358979323846264338327950288

typedef struct bare_file { int unused; } FILE;
FILE bare_stdout, bare_stderr;
FILE *stdout = &bare_stdout;
FILE *stderr = &bare_stderr;

typedef struct allocation_header { size_t size; } allocation_header;
static unsigned char heap[HEAP_BYTES] __attribute__((aligned(64)));
static size_t heap_offset;
static int failed;

void *memset(void *dst, int value, size_t n)
{
    unsigned char *d = dst;
    while (n--) *d++ = (unsigned char)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *malloc(size_t size)
{
    size_t total = (sizeof(allocation_header) + size + 63u) & ~(size_t)63u;
    allocation_header *h;
    if (total > HEAP_BYTES - heap_offset) { failed = 1; return 0; }
    h = (allocation_header *)&heap[heap_offset];
    heap_offset += total;
    h->size = size;
    return h + 1;
}

void *calloc(size_t count, size_t size)
{
    size_t total;
    void *p;
    if (count && size > (size_t)-1 / count) return 0;
    total = count * size;
    p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *old, size_t size)
{
    allocation_header *h;
    void *p;
    size_t n;
    if (!old) return malloc(size);
    h = (allocation_header *)old - 1;
    p = malloc(size);
    if (!p) return 0;
    n = h->size < size ? h->size : size;
    memcpy(p, old, n);
    return p;
}

void free(void *ptr) { (void)ptr; }
int runtime_has_failed(void) { return failed; }
void exit(int status) { failed = status != 0; }

int atoi(const char *p)
{
    int sign = 1, value = 0;
    if (*p == '-') { sign = -1; p++; }
    while (*p >= '0' && *p <= '9') value = value * 10 + *p++ - '0';
    return sign * value;
}

double sqrt(double x)
{
    double result;
    __asm__ volatile ("fsqrt.d %0, %1" : "=f"(result) : "f"(x));
    return result;
}

static double reduce_angle(double x)
{
    while (x > PI) x -= 2.0 * PI;
    while (x < -PI) x += 2.0 * PI;
    return x;
}

double sin(double x)
{
    double x2, term, sum;
    int n;
    x = reduce_angle(x);
    x2 = x * x;
    term = sum = x;
    for (n = 1; n < 12; n++) {
        term *= -x2 / ((2.0*n) * (2.0*n + 1.0));
        sum += term;
    }
    return sum;
}

double cos(double x)
{
    double x2, term = 1.0, sum = 1.0;
    int n;
    x = reduce_angle(x);
    x2 = x * x;
    for (n = 1; n < 12; n++) {
        term *= -x2 / ((2.0*n - 1.0) * (2.0*n));
        sum += term;
    }
    return sum;
}

double log(double x)
{
    int exponent = 0, n;
    double y, y2, term, sum = 0.0;
    if (x <= 0.0) { failed = 1; return 0.0; }
    while (x > 2.0) { x *= 0.5; exponent++; }
    while (x < 1.0) { x *= 2.0; exponent--; }
    y = (x - 1.0) / (x + 1.0);
    y2 = y * y;
    term = y;
    for (n = 1; n < 24; n += 2) {
        sum += term / n;
        term *= y2;
    }
    return 2.0 * sum + exponent * 0.69314718055994530942;
}

int printf(const char *format, ...) { (void)format; return 0; }
int fprintf(FILE *file, const char *format, ...) { (void)file; (void)format; return 0; }
FILE *fopen(const char *name, const char *mode) { (void)name; (void)mode; return 0; }
int fclose(FILE *file) { (void)file; return 0; }
size_t fread(void *p, size_t s, size_t n, FILE *f) { (void)p;(void)s;(void)n;(void)f;return 0; }
size_t fwrite(const void *p,size_t s,size_t n,FILE *f){(void)p;(void)s;(void)n;(void)f;return 0;}
int feof(FILE *file) { (void)file; return 1; }
int fgetc(FILE *file) { (void)file; return -1; }
void perror(const char *message) { (void)message; }
char *getenv(const char *name) { (void)name; return 0; }
int atexit(void (*function)(void)) { (void)function; return 0; }
long time(long *value) { if (value) *value = 1; return 1; }
int rand(void) { return 1; }

float _Complex __mulsc3(float a, float b, float c, float d)
{
    float _Complex result;
    __real__ result = a * c - b * d;
    __imag__ result = a * d + b * c;
    return result;
}

float _Complex __divsc3(float a, float b, float c, float d)
{
    float _Complex result;
    float denominator = c * c + d * d;
    __real__ result = (a * c + b * d) / denominator;
    __imag__ result = (b * c - a * d) / denominator;
    return result;
}
