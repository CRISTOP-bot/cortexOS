#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "tty_line.h"

static char echoed[128];
static size_t echoed_len;
static int signal_seen;

static void capture(void *ctx, unsigned char byte)
{
	(void)ctx;
	if (echoed_len < sizeof(echoed)) echoed[echoed_len++] = (char)byte;
}
static void signal_capture(void *ctx, int signal)
{
	(void)ctx;
	signal_seen = signal;
}

int main(void)
{
	struct tty_line line;
	struct termios raw;
	char output[32];
	int n;

	tty_line_init(&line);
	tty_line_feed(&line, 'a', capture, signal_capture, 0);
	tty_line_feed(&line, 'b', capture, signal_capture, 0);
	tty_line_feed(&line, 8, capture, signal_capture, 0);
	tty_line_feed(&line, 'c', capture, signal_capture, 0);
	tty_line_feed(&line, '\n', capture, signal_capture, 0);
	n = tty_line_read(&line, output, sizeof(output));
	assert(n == 3 && memcmp(output, "ac\n", 3) == 0);
	assert(echoed_len >= 6 && echoed[0] == 'a' && echoed[1] == 'b');

	raw = line.termios;
	raw.c_lflag &= ~(ICANON | ECHO | ISIG);
	tty_line_set_termios(&line, &raw);
	tty_line_feed(&line, 'x', 0, signal_capture, 0);
	tty_line_feed(&line, 'y', 0, signal_capture, 0);
	n = tty_line_read(&line, output, sizeof(output));
	assert(n == 2 && memcmp(output, "xy", 2) == 0);

	raw.c_lflag |= ISIG;
	raw.c_cc[VINTR] = 3;
	tty_line_set_termios(&line, &raw);
	signal_seen = 0;
	tty_line_feed(&line, 3, 0, signal_capture, 0);
	assert(signal_seen == 2);
	return 0;
}
