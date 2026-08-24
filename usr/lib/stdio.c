#include <stdarg.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/unistd.h>

static FILE stdin_object = { 0 };
static FILE stdout_object = { 1 };
static FILE stderr_object = { 2 };
FILE *__stdin = &stdin_object;
FILE *__stdout = &stdout_object;
FILE *__stderr = &stderr_object;

int fgetc(FILE *stream)
{
	unsigned char c;
	return read(stream->fd, &c, 1) == 1 ? c : -1;
}

int fputc(int value, FILE *stream)
{
	unsigned char c = (unsigned char)value;
	return write(stream->fd, &c, 1) == 1 ? c : -1;
}

int fputs(const char *text, FILE *stream)
{
	return (int)write(stream->fd, text, strlen(text));
}

int puts(const char *text)
{
	int result = fputs(text, stdout);
	if (result < 0 || fputc('\n', stdout) < 0)
		return -1;
	return result + 1;
}

static void output_char(char *buffer, size_t size, size_t *used, char c)
{
	if (buffer && *used + 1 < size)
		buffer[*used] = c;
	++*used;
}

static void output_text(char *buffer, size_t size, size_t *used, const char *text)
{
	while (*text)
		output_char(buffer, size, used, *text++);
}

static void output_number(char *buffer, size_t size, size_t *used, long value)
{
	char digits[32];
	int n = 0;
	unsigned long number;
	if (value < 0) {
		output_char(buffer, size, used, '-');
		number = (unsigned long)(-value);
	} else {
		number = (unsigned long)value;
	}
	if (number == 0)
		digits[n++] = '0';
	while (number) {
		digits[n++] = (char)('0' + number % 10);
		number /= 10;
	}
	while (n)
		output_char(buffer, size, used, digits[--n]);
}

static int format(char *buffer, size_t size, const char *format_text, va_list args)
{
	size_t used = 0;
	for (size_t i = 0; format_text[i]; ++i) {
		if (format_text[i] != '%') {
			output_char(buffer, size, &used, format_text[i]);
			continue;
		}
		switch (format_text[++i]) {
		case '%': output_char(buffer, size, &used, '%'); break;
		case 'c': output_char(buffer, size, &used, (char)va_arg(args, int)); break;
		case 's': output_text(buffer, size, &used, va_arg(args, const char *)); break;
		case 'd': case 'i': output_number(buffer, size, &used, va_arg(args, int)); break;
		case 'l':
			if (format_text[i + 1] == 'd') {
				++i;
				output_number(buffer, size, &used, va_arg(args, long));
			}
			break;
		default: output_char(buffer, size, &used, '?'); break;
		}
	}
	if (buffer && size)
		buffer[used < size ? used : size - 1] = '\0';
	return (int)used;
}

int snprintf(char *buffer, size_t size, const char *format_text, ...)
{
	va_list args;
	int result;
	va_start(args, format_text);
	result = format(buffer, size, format_text, args);
	va_end(args);
	return result;
}

int printf(const char *format_text, ...)
{
	char buffer[1024];
	va_list args;
	int result;
	va_start(args, format_text);
	result = format(buffer, sizeof(buffer), format_text, args);
	va_end(args);
	if (result > 0)
		write(1, buffer, strlen(buffer));
	return result;
}

int fflush(FILE *stream) { (void)stream; return 0; }
int fclose(FILE *stream) { return close(stream->fd); }
