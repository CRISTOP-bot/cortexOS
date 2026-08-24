#ifndef CORTEXOS_TTY_H
#define CORTEXOS_TTY_H

#include <stddef.h>
#include <stdbool.h>
#include "abi/termios.h"

#define CORTEXOS_TTY_CONSOLE 0
#define CORTEXOS_TTY0 0

void tty_init(void);
int tty_read(int tty, void *buffer, size_t count);
int tty_write(int tty, const void *buffer, size_t count);
int tty_get_termios(struct termios *termios);
int tty_set_termios(const struct termios *termios);
int tty_get_foreground(void);
int tty_set_foreground(int pgid);
void tty_signal_foreground(int signal);
bool tty_is_device_path(const char *path);

#endif
