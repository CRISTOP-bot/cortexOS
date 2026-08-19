#ifndef CORTEXOS_STDLIB_H
#define CORTEXOS_STDLIB_H
#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void exit(int status) __attribute__((noreturn));
long strtol(const char *text, char **end, int base);
int atoi(const char *text);

#endif
