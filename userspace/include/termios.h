#ifndef CORTEXOS_TERMIOS_H
#define CORTEXOS_TERMIOS_H
#include <stdint.h>
typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
struct termios {
    tcflag_t c_iflag, c_oflag, c_cflag, c_lflag;
    cc_t c_line;
    cc_t c_cc[32];
};

/* Linux-compatible request numbers used by the small CortexOS ABI. */
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define NCCS 32

/* Input flags. */
#define IGNBRK 0x0001
#define BRKINT 0x0002
#define IGNPAR 0x0004
#define INPCK  0x0010
#define ISTRIP 0x0020
#define INLCR  0x0040
#define IGNCR  0x0080
#define ICRNL  0x0100
#define IXON   0x0400
/* Output flags. */
#define OPOST  0x0001
#define ONLCR  0x0004
/* Control flags. */
#define CREAD  0x0080
#define CS8    0x0030
/* Local flags. */
#define ISIG   0x0001
#define ICANON 0x0002
#define ECHO   0x0008
#define ECHOE  0x0010
#define ECHOK  0x0020
#define ECHONL 0x0040

#define VINTR  0
#define VQUIT  1
#define VERASE 2
#define VKILL  3
#define VEOF   4
#define VTIME  5
#define VMIN   6

#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, int pgrp);
#endif
