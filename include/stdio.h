#ifndef CORTEXOS_STDIO_H
#define CORTEXOS_STDIO_H
#include <stddef.h>

typedef struct { int fd; } FILE;
extern FILE *__stdin;
extern FILE *__stdout;
extern FILE *__stderr;
#define stdin (__stdin)
#define stdout (__stdout)
#define stderr (__stderr)

int fgetc(FILE *stream);
int fputc(int value, FILE *stream);
int fputs(const char *text, FILE *stream);
int puts(const char *text);
int printf(const char *format, ...);
int snprintf(char *buffer, size_t size, const char *format, ...);
int fflush(FILE *stream);
int fclose(FILE *stream);

#endif
