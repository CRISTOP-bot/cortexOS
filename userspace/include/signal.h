#ifndef CORTEXOS_SIGNAL_H
#define CORTEXOS_SIGNAL_H

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGKILL 9
#define SIGTERM 15
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)

typedef void (*sighandler_t)(int);
struct sigaction {
    sighandler_t sa_handler;
    unsigned long sa_flags;
    unsigned long sa_mask;
};
int kill(int pid, int sig);
int sigaction(int sig, const struct sigaction *action,
              struct sigaction *old_action);
#endif
