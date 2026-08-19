#include <stddef.h>
#include <string.h>

void *memcpy(void *dst, const void *src, size_t count)
{
	unsigned char *d = dst;
	const unsigned char *s = src;
	for (size_t i = 0; i < count; ++i)
		d[i] = s[i];
	return dst;
}

void *memset(void *dst, int value, size_t count)
{
	unsigned char *d = dst;
	for (size_t i = 0; i < count; ++i)
		d[i] = (unsigned char)value;
	return dst;
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
