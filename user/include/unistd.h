#ifndef NUCLEOS_UNISTD_H
#define NUCLEOS_UNISTD_H
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

#endif

