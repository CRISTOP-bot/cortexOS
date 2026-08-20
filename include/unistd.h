#ifndef CORTEXOS_UNISTD_H
#define CORTEXOS_UNISTD_H
#include <stddef.h>
#include <sys/types.h>

ssize_t read(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
int close(int fd);
int dup2(int old_fd, int new_fd);
int pipe(int pipe_fds[2]);
int isatty(int fd);
off_t lseek(int fd, off_t offset, int whence);
int fork(void);
int execve(const char *path, char *const argv[], char *const envp[]);
int wait(int *status);
int waitpid(pid_t pid, int *status, int options);
int getpid(void);
void _exit(int status) __attribute__((noreturn));
void *sbrk(intptr_t increment);
char *getcwd(char *buffer, size_t size);
int chdir(const char *path);
int ioctl(int fd, unsigned long request, ...);
int setsid(void);
int getsid(int pid);
int setpgid(int pid, int pgid);
int getpgrp(void);
int mount(const char *source, const char *target, const char *filesystem,
          unsigned long flags, const void *data);
int umount(const char *target);
int mkdir(const char *path, unsigned int mode);
int unlink(const char *path);
int access(const char *path, int mode);
int getuid(void);
int geteuid(void);
int getgid(void);
int getegid(void);

/* Signal return: called at the end of a signal handler to restore the
 * interrupted context. Does not return; control transfers to the point
 * where the signal interrupted execution. */
void sigreturn(void) __attribute__((noreturn));

#endif

