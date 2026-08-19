#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

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

void free(void *ptr)
{
	/* CortexOS starts with a monotonic heap; individual blocks are retained. */
	(void)ptr;
}

void exit(int status)
{
	_exit(status);
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
