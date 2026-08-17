#ifndef BARE_STDIO_H
#define BARE_STDIO_H
#include <stddef.h>
typedef struct bare_file FILE;
extern FILE *stdout;
extern FILE *stderr;
FILE *fopen(const char *, const char *);
char *fgets(char *, int, FILE *);
int fclose(FILE *);
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sscanf(const char *, const char *, ...);
int fflush(FILE *);
#endif
