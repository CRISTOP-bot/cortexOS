#ifndef CORTEXOS_TTY_LINE_H
#define CORTEXOS_TTY_LINE_H

#include <stddef.h>
#include <stdbool.h>
#include "abi/termios.h"

#define TTY_LINE_BUFFER_SIZE 4096

typedef void (*tty_line_emit_fn)(void *context, unsigned char byte);
typedef void (*tty_line_signal_fn)(void *context, int signal);

/* The line discipline is deliberately independent of the console and
 * keyboard drivers so it can be tested on a host. */
struct tty_line {
	struct termios termios;
	unsigned char input[TTY_LINE_BUFFER_SIZE];
	size_t input_head;
	size_t input_tail;
	size_t input_count;
	unsigned char line[TTY_LINE_BUFFER_SIZE];
	size_t line_length;
	bool eof_pending;
};

void tty_line_init(struct tty_line *line);
void tty_line_set_termios(struct tty_line *line, const struct termios *termios);
int tty_line_feed(struct tty_line *line, unsigned char byte,
		tty_line_emit_fn emit, tty_line_signal_fn signal, void *context);
int tty_line_read(struct tty_line *line, void *buffer, size_t count);
int tty_line_write(const struct tty_line *line, const void *buffer, size_t count,
		tty_line_emit_fn emit, void *context);

#endif
