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
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define NCCS 32
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, int pgrp);
#endif
