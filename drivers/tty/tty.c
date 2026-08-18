#include "tty.h"
#include "tty_line.h"
#include "keyboard.h"
#include "console.h"
#include "process.h"
#include "kstring.h"

static struct tty_line console_tty;
static int foreground_pgid;

static void tty_console_emit(void *context, unsigned char byte)
{
	(void)context;
	console_putchar((char)byte);
}

static void tty_signal(void *context, int signal)
{
	(void)context;
	process_tty_signal(signal);
}

void tty_init(void)
{
	tty_line_init(&console_tty);
	foreground_pgid = 0;
}

int tty_read(int tty, void *buffer, size_t count)
{
	int result;
	if (tty != CORTEXOS_TTY_CONSOLE || !buffer || count == 0) return 0;
	for (;;) {
		result = tty_line_read(&console_tty, buffer, count);
		if (result >= 0) return result;
		/* Keyboard translation is kept in the PS/2 driver. Feeding one
		 * translated character at a time preserves the old shell path while
		 * giving user processes a real line discipline. */
		tty_line_feed(&console_tty, (unsigned char)keyboard_read_char(),
			tty_console_emit, tty_signal, 0);
	}
}

int tty_write(int tty, const void *buffer, size_t count)
{
	if (tty != CORTEXOS_TTY_CONSOLE || (!buffer && count)) return -1;
	return tty_line_write(&console_tty, buffer, count, tty_console_emit, 0);
}

int tty_get_termios(struct termios *termios)
{
	if (!termios) return -1;
	*termios = console_tty.termios;
	return 0;
}

int tty_set_termios(const struct termios *termios)
{
	if (!termios) return -1;
	tty_line_set_termios(&console_tty, termios);
	return 0;
}

int tty_get_foreground(void) { return foreground_pgid; }
int tty_set_foreground(int pgid)
{
	if (pgid <= 0) return -1;
	foreground_pgid = pgid;
	return 0;
}

void tty_signal_foreground(int signal)
{
	process_tty_signal(signal);
}

bool tty_is_device_path(const char *path)
{
	if (!path) return false;
	return kstrcmp(path, "/dev/console") == 0 ||
		kstrcmp(path, "dev/console") == 0 ||
		kstrcmp(path, "/dev/tty") == 0 ||
		kstrcmp(path, "dev/tty") == 0 ||
		kstrcmp(path, "/dev/tty0") == 0 ||
		kstrcmp(path, "dev/tty0") == 0;
}
