#ifndef CORTEXOS_SYS_WAIT_H
#define CORTEXOS_SYS_WAIT_H

#define WNOHANG 1

/* CortexOS's wait/waitpid ABI is NOT the Linux bit-packed status word.
 * ipc/process/process.c stores exit_code directly: a normal exit() leaves
 * exit_code == the exit status (0-127), and death by signal leaves
 * exit_code == 128 + signal number (see process_exit / the kill path).
 * These macros match that raw encoding rather than POSIX's <<8 layout, so
 * do not reuse Linux's WEXITSTATUS()/WIFEXITED() bit math here. */
#define WIFSIGNALED(status)  ((status) >= 128 && (status) < 128 + 64)
#define WTERMSIG(status)     ((status) - 128)
#define WIFEXITED(status)    (!WIFSIGNALED(status))
#define WEXITSTATUS(status)  (status)
#define WIFSTOPPED(status)   (0)
#define WSTOPSIG(status)     (0)

#endif
