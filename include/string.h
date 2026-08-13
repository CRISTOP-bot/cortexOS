#ifndef CORTEXOS_STRING_H
#define CORTEXOS_STRING_H
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t count);
void *memset(void *dst, int value, size_t count);
size_t strlen(const char *text);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t count);

#endif
