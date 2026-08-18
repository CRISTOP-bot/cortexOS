#include "tty_line.h"

#define SIGINT 2

static void emit_byte(tty_line_emit_fn emit, void *context, unsigned char byte)
{
	if (emit) emit(context, byte);
}

static void enqueue(struct tty_line *line, unsigned char byte)
{
	if (line->input_count >= TTY_LINE_BUFFER_SIZE) return;
	line->input[line->input_head] = byte;
	line->input_head = (line->input_head + 1) % TTY_LINE_BUFFER_SIZE;
	++line->input_count;
}

static void commit_line(struct tty_line *line)
{
	size_t i;
	for (i = 0; i < line->line_length; ++i) enqueue(line, line->line[i]);
	line->line_length = 0;
}

void tty_line_init(struct tty_line *line)
{
	if (!line) return;
	line->input_head = line->input_tail = line->input_count = 0;
	line->line_length = 0;
	line->eof_pending = false;
	line->termios.c_iflag = ICRNL | IXON;
	line->termios.c_oflag = OPOST | ONLCR;
	line->termios.c_cflag = CREAD | CS8;
	line->termios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHONL;
	line->termios.c_line = 0;
	for (size_t i = 0; i < NCCS; ++i) line->termios.c_cc[i] = 0;
	line->termios.c_cc[VINTR] = 3;
	line->termios.c_cc[VQUIT] = 28;
	line->termios.c_cc[VERASE] = 8;
	line->termios.c_cc[VKILL] = 21;
	line->termios.c_cc[VEOF] = 4;
	line->termios.c_cc[VMIN] = 1;
}

void tty_line_set_termios(struct tty_line *line, const struct termios *termios)
{
	if (line && termios) line->termios = *termios;
}

int tty_line_feed(struct tty_line *line, unsigned char byte,
	tty_line_emit_fn emit, tty_line_signal_fn signal, void *context)
{
	const struct termios *t;
	if (!line) return -1;
	t = &line->termios;
	if (byte == '\r' && (t->c_iflag & ICRNL)) byte = '\n';
	else if (byte == '\n' && (t->c_iflag & INLCR)) byte = '\r';
	if ((t->c_lflag & ISIG) && byte == t->c_cc[VINTR]) {
		if (signal) signal(context, SIGINT);
		line->line_length = 0;
		if (t->c_lflag & ECHO) {
			emit_byte(emit, context, '^');
			emit_byte(emit, context, 'C');
			emit_byte(emit, context, '\n');
		}
		return 0;
	}
	if (!(t->c_lflag & ICANON)) {
		enqueue(line, byte);
		if (t->c_lflag & ECHO) emit_byte(emit, context, byte);
		return 0;
	}
	if (byte == t->c_cc[VERASE]) {
		if (line->line_length) {
			--line->line_length;
			if (t->c_lflag & ECHOE) {
				emit_byte(emit, context, '\b');
				emit_byte(emit, context, ' ');
				emit_byte(emit, context, '\b');
			} else if (t->c_lflag & ECHO) emit_byte(emit, context, byte);
		}
		return 0;
	}
	if (byte == t->c_cc[VKILL]) {
		while (line->line_length) {
			--line->line_length;
			if (t->c_lflag & ECHOE) {
				emit_byte(emit, context, '\b');
				emit_byte(emit, context, ' ');
				emit_byte(emit, context, '\b');
			}
		}
		return 0;
	}
	if (byte == t->c_cc[VEOF]) {
		if (line->line_length) commit_line(line);
		else line->eof_pending = true;
		return 0;
	}
	if (line->line_length < TTY_LINE_BUFFER_SIZE - 1)
		line->line[line->line_length++] = byte;
	if ((t->c_lflag & ECHO) || ((t->c_lflag & ECHONL) && byte == '\n'))
		emit_byte(emit, context, byte);
	if (byte == '\n') commit_line(line);
	return 0;
}

int tty_line_read(struct tty_line *line, void *buffer, size_t count)
{
	unsigned char *out = (unsigned char *)buffer;
	size_t n = 0;
	if (!line || !buffer || count == 0) return 0;
	if (line->eof_pending && line->input_count == 0) {
		line->eof_pending = false;
		return 0;
	}
	while (n < count && line->input_count) {
		out[n++] = line->input[line->input_tail];
		line->input_tail = (line->input_tail + 1) % TTY_LINE_BUFFER_SIZE;
		--line->input_count;
	}
	return n ? (int)n : -1;
}

int tty_line_write(const struct tty_line *line, const void *buffer, size_t count,
	tty_line_emit_fn emit, void *context)
{
	const unsigned char *src = (const unsigned char *)buffer;
	if (!line || !buffer) return -1;
	for (size_t i = 0; i < count; ++i) {
		if ((line->termios.c_oflag & OPOST) && src[i] == '\n' &&
				(line->termios.c_oflag & ONLCR)) emit_byte(emit, context, '\r');
		emit_byte(emit, context, src[i]);
	}
	return (int)count;
}
