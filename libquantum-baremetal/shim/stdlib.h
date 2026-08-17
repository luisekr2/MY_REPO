#ifndef BARE_STDLIB_H
#define BARE_STDLIB_H
#include <stddef.h>
void *malloc(size_t); void *calloc(size_t,size_t); void *realloc(void*,size_t);
void free(void *); void exit(int); int atoi(const char *); int rand(void);
char *getenv(const char *); int atexit(void (*)(void));
#endif
