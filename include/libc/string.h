#ifndef CORTEXOS_STRING_H
#define CORTEXOS_STRING_H
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t count);
void *memmove(void *dst, const void *src, size_t count);
void *memset(void *dst, int value, size_t count);
int memcmp(const void *left, const void *right, size_t count);
void *memchr(const void *buffer, int value, size_t count);
size_t strlen(const char *text);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t count);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t count);
char *strchr(const char *text, int character);
char *strrchr(const char *text, int character);
char *strstr(const char *haystack, const char *needle);
size_t strspn(const char *text, const char *accept);
size_t strcspn(const char *text, const char *reject);
char *strtok(char *text, const char *delimiters);
char *strdup(const char *text);
char *strerror(int error_number);

#endif
