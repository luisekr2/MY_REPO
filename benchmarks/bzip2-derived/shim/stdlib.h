#ifndef BZIP2_DERIVED_STDLIB_H
#define BZIP2_DERIVED_STDLIB_H

typedef __SIZE_TYPE__ size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

void *malloc(size_t size);
void free(void *ptr);
void exit(int status);

#endif
