#ifndef CORTEXOS_STDLIB_H
#define CORTEXOS_STDLIB_H
#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void exit(int status) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
long strtol(const char *text, char **end, int base);
int atoi(const char *text);
long atol(const char *text);
int abs(int value);
long labs(long value);

/* environ is populated by crt0 from the envp the kernel places on the
 * initial user stack. getenv/setenv/unsetenv are a minimal, non-POSIX-strict
 * implementation over that array: setenv only grows the table up to
 * CORTEXOS_ENVIRON_MAX and never reclaims slots on unsetenv. That is enough
 * for coreutils/bash startup but should be revisited once a real heap
 * (free-capable malloc) exists. */
extern char **environ;
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

#endif
