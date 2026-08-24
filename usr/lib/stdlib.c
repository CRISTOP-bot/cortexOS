#include <stddef.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <libc/unistd.h>

struct allocation {
	size_t size;
};

void *malloc(size_t size)
{
	struct allocation *block;
	if (size == 0)
		size = 1;
	block = (struct allocation *)sbrk((intptr_t)(size + sizeof(*block)));
	if (block == (void *)-1)
		return (void *)0;
	block->size = size;
	return block + 1;
}

void *calloc(size_t count, size_t size)
{
	size_t total = count * size;
	void *ptr;
	/* Guard the classic overflow: count * size wrapping past SIZE_MAX would
	 * otherwise hand back an undersized, zeroed-looking buffer. */
	if (count != 0 && total / count != size)
		return NULL;
	ptr = malloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

void *realloc(void *ptr, size_t size)
{
	struct allocation *block;
	void *new_ptr;
	if (!ptr)
		return malloc(size);
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	block = (struct allocation *)ptr - 1;
	if (size <= block->size)
		return ptr;
	new_ptr = malloc(size);
	if (!new_ptr)
		return NULL;
	memcpy(new_ptr, ptr, block->size);
	free(ptr);
	return new_ptr;
}

void free(void *ptr)
{
	/* CortexOS starts with a monotonic bump heap (sbrk-only, no freelist);
	 * individual blocks are retained until a real allocator replaces this. */
	(void)ptr;
}

void exit(int status)
{
	_exit(status);
}

void abort(void)
{
	_exit(134); /* 128 + SIGABRT(6), matching the kernel's exit_code ABI. */
}

long strtol(const char *text, char **end, int base)
{
	long value = 0;
	int sign = 1;
	if (!text)
		return 0;
	while (*text == ' ' || *text == '\t' || *text == '\n')
		++text;
	if (*text == '-') {
		sign = -1;
		++text;
	}
	if (base == 0) {
		base = 10;
		if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
			base = 16;
			text += 2;
		}
	}
	while (*text) {
		int digit;
		if (*text >= '0' && *text <= '9')
			digit = *text - '0';
		else if (*text >= 'a' && *text <= 'f')
			digit = *text - 'a' + 10;
		else if (*text >= 'A' && *text <= 'F')
			digit = *text - 'A' + 10;
		else
			break;
		if (digit >= base)
			break;
		value = value * base + digit;
		++text;
	}
	if (end)
		*end = (char *)text;
	return value * sign;
}

int atoi(const char *text)
{
	return (int)strtol(text, (char **)0, 10);
}

long atol(const char *text)
{
	return strtol(text, (char **)0, 10);
}

int abs(int value)
{
	return value < 0 ? -value : value;
}

long labs(long value)
{
	return value < 0 ? -value : value;
}

/* Populated by crt0 from the envp the kernel places on the initial stack. */
char **environ = NULL;

#define CORTEXOS_ENVIRON_MAX 128
static char *environ_storage[CORTEXOS_ENVIRON_MAX + 1];
static int environ_owned = 0; /* becomes 1 once we've switched off crt0's array */

static size_t environ_count(void)
{
	size_t n = 0;
	if (environ) {
		while (environ[n])
			++n;
	}
	return n;
}

static size_t name_len(const char *name)
{
	size_t n = 0;
	while (name[n] && name[n] != '=')
		++n;
	return n;
}

char *getenv(const char *name)
{
	size_t len = name_len(name);
	if (!environ)
		return NULL;
	for (size_t i = 0; environ[i]; ++i) {
		if (strncmp(environ[i], name, len) == 0 && environ[i][len] == '=')
			return environ[i] + len + 1;
	}
	return NULL;
}

int setenv(const char *name, const char *value, int overwrite)
{
	size_t len = name_len(name);
	size_t count;
	char *entry;

	if (!name || len == 0)
		return -1;

	/* First mutation: copy crt0's (possibly read-only/short) envp array
	 * into our own writable, fixed-capacity table. */
	if (!environ_owned) {
		size_t n = environ_count();
		if (n > CORTEXOS_ENVIRON_MAX)
			n = CORTEXOS_ENVIRON_MAX;
		for (size_t i = 0; i < n; ++i)
			environ_storage[i] = environ[i];
		environ_storage[n] = NULL;
		environ = environ_storage;
		environ_owned = 1;
	}

	for (size_t i = 0; environ[i]; ++i) {
		if (strncmp(environ[i], name, len) == 0 && environ[i][len] == '=') {
			if (!overwrite)
				return 0;
			entry = malloc(len + 1 + strlen(value) + 1);
			if (!entry)
				return -1;
			memcpy(entry, name, len);
			entry[len] = '=';
			strcpy(entry + len + 1, value);
			environ[i] = entry;
			return 0;
		}
	}

	count = environ_count();
	if (count >= CORTEXOS_ENVIRON_MAX)
		return -1; /* Table full; a real heap-backed array comes later. */

	entry = malloc(len + 1 + strlen(value) + 1);
	if (!entry)
		return -1;
	memcpy(entry, name, len);
	entry[len] = '=';
	strcpy(entry + len + 1, value);
	environ[count] = entry;
	environ[count + 1] = NULL;
	return 0;
}

int unsetenv(const char *name)
{
	size_t len = name_len(name);
	if (!environ)
		return 0;
	for (size_t i = 0; environ[i]; ++i) {
		if (strncmp(environ[i], name, len) == 0 && environ[i][len] == '=') {
			size_t j = i;
			for (; environ[j]; ++j)
				environ[j] = environ[j + 1];
			return 0;
		}
	}
	return 0;
}
