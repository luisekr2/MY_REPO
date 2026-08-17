#ifndef BARE_STDIO_H
#define BARE_STDIO_H
#include <stddef.h>
typedef struct bare_file FILE;
extern FILE *stdout, *stderr;
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int feof(FILE *); int fgetc(FILE *); void perror(const char *);
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define EOF (-1)
#endif
