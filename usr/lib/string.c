#include <stddef.h>
#include <string.h>
#include <stdlib.h>

void *memcpy(void *dst, const void *src, size_t count)
{
	unsigned char *d = dst;
	const unsigned char *s = src;
	for (size_t i = 0; i < count; ++i)
		d[i] = s[i];
	return dst;
}

/* memmove must remain correct when the source and destination regions
 * overlap; coreutils/bash rely on this (e.g. shifting buffers in place). */
void *memmove(void *dst, const void *src, size_t count)
{
	unsigned char *d = dst;
	const unsigned char *s = src;
	if (d == s || count == 0)
		return dst;
	if (d < s) {
		for (size_t i = 0; i < count; ++i)
			d[i] = s[i];
	} else {
		for (size_t i = count; i != 0; --i)
			d[i - 1] = s[i - 1];
	}
	return dst;
}

void *memset(void *dst, int value, size_t count)
{
	unsigned char *d = dst;
	for (size_t i = 0; i < count; ++i)
		d[i] = (unsigned char)value;
	return dst;
}

int memcmp(const void *left, const void *right, size_t count)
{
	const unsigned char *l = left;
	const unsigned char *r = right;
	for (size_t i = 0; i < count; ++i) {
		if (l[i] != r[i])
			return (int)l[i] - (int)r[i];
	}
	return 0;
}

void *memchr(const void *buffer, int value, size_t count)
{
	const unsigned char *b = buffer;
	unsigned char target = (unsigned char)value;
	for (size_t i = 0; i < count; ++i) {
		if (b[i] == target)
			return (void *)(b + i);
	}
	return NULL;
}

size_t strlen(const char *text)
{
	size_t n = 0;
	while (text && text[n])
		++n;
	return n;
}

int strcmp(const char *left, const char *right)
{
	while (*left && *left == *right) {
		++left;
		++right;
	}
	return (unsigned char)*left - (unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		if (left[i] != right[i] || !left[i])
			return (unsigned char)left[i] - (unsigned char)right[i];
	}
	return 0;
}

char *strcpy(char *dst, const char *src)
{
	char *start = dst;
	while ((*dst++ = *src++) != '\0')
		;
	return start;
}

char *strncpy(char *dst, const char *src, size_t count)
{
	size_t i = 0;
	for (; i < count && src[i]; ++i)
		dst[i] = src[i];
	for (; i < count; ++i)
		dst[i] = 0;
	return dst;
}

char *strcat(char *dst, const char *src)
{
	char *end = dst + strlen(dst);
	strcpy(end, src);
	return dst;
}

char *strncat(char *dst, const char *src, size_t count)
{
	char *end = dst + strlen(dst);
	size_t i = 0;
	for (; i < count && src[i]; ++i)
		end[i] = src[i];
	end[i] = '\0';
	return dst;
}

char *strchr(const char *text, int character)
{
	char c = (char)character;
	for (; *text; ++text) {
		if (*text == c)
			return (char *)text;
	}
	return c == '\0' ? (char *)text : NULL;
}

char *strrchr(const char *text, int character)
{
	char c = (char)character;
	const char *last = NULL;
	for (; *text; ++text) {
		if (*text == c)
			last = text;
	}
	return c == '\0' ? (char *)text : (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
	size_t needle_len = strlen(needle);
	if (needle_len == 0)
		return (char *)haystack;
	for (; *haystack; ++haystack) {
		if (strncmp(haystack, needle, needle_len) == 0)
			return (char *)haystack;
	}
	return NULL;
}

static int char_in_set(char c, const char *set)
{
	for (; *set; ++set) {
		if (*set == c)
			return 1;
	}
	return 0;
}

size_t strspn(const char *text, const char *accept)
{
	size_t n = 0;
	while (text[n] && char_in_set(text[n], accept))
		++n;
	return n;
}

size_t strcspn(const char *text, const char *reject)
{
	size_t n = 0;
	while (text[n] && !char_in_set(text[n], reject))
		++n;
	return n;
}

/* Minimal strtok: not thread/signal safe (single static cursor), matching
 * the rest of this early libc. Good enough for shell/coreutils tokenizing
 * on a single-threaded userspace program. */
static char *strtok_cursor = NULL;

char *strtok(char *text, const char *delimiters)
{
	if (text)
		strtok_cursor = text;
	if (!strtok_cursor)
		return NULL;

	strtok_cursor += strspn(strtok_cursor, delimiters);
	if (*strtok_cursor == '\0') {
		strtok_cursor = NULL;
		return NULL;
	}

	char *token = strtok_cursor;
	strtok_cursor += strcspn(strtok_cursor, delimiters);
	if (*strtok_cursor) {
		*strtok_cursor = '\0';
		++strtok_cursor;
	} else {
		strtok_cursor = NULL;
	}
	return token;
}

char *strdup(const char *text)
{
	size_t len = strlen(text) + 1;
	char *copy = malloc(len);
	if (!copy)
		return NULL;
	memcpy(copy, text, len);
	return copy;
}

/* Small fixed table; grows as syscalls start returning richer errno values.
 * Keep in sync with include/errno.h. */
char *strerror(int error_number)
{
	switch (error_number) {
	case 0: return "Success";
	case 2: return "No such file or directory";
	case 9: return "Bad file descriptor";
	case 11: return "Resource temporarily unavailable";
	case 12: return "Cannot allocate memory";
	case 13: return "Permission denied";
	case 22: return "Invalid argument";
	case 38: return "Function not implemented";
	default: return "Unknown error";
	}
}
