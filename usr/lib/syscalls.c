#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <nucleos_syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int errno;

static long result(long value)
{
	if (value < 0)
		errno = (int)-value;
	return value;
}

ssize_t read(int fd, void *buffer, size_t count)
{
	return (ssize_t)result(__cortexos_syscall3(N_SYS_READ, fd, buffer, count));
}

ssize_t write(int fd, const void *buffer, size_t count)
{
	return (ssize_t)result(__cortexos_syscall3(N_SYS_WRITE, fd, buffer, count));
}

int open(const char *path, int flags, ...)
{
	va_list args;
	int mode = 0;
	long value;
	va_start(args, flags);
	if (flags & O_CREAT)
		mode = va_arg(args, int);
	va_end(args);
	value = __cortexos_syscall3(N_SYS_OPEN, path, flags, mode);
	return (int)result(value);
}

int fcntl(int fd, int command, ...)
{
	va_list args;
	long value = 0;
	va_start(args, command);
	if (command == F_SETFL)
		value = va_arg(args, int);
	va_end(args);
	return (int)result(__cortexos_syscall3(N_SYS_FCNTL, fd, command, value));
}

int close(int fd)
{
	return (int)result(__cortexos_syscall1(N_SYS_CLOSE, fd));
}

int dup2(int old_fd, int new_fd)
{
	return (int)result(__cortexos_syscall2(N_SYS_DUP2, old_fd, new_fd));
}

int pipe(int pipe_fds[2])
{
	return (int)result(__cortexos_syscall1(N_SYS_PIPE, pipe_fds));
}

int isatty(int fd)
{
	return (int)result(__cortexos_syscall1(N_SYS_ISATTY, fd));
}

off_t lseek(int fd, off_t offset, int whence)
{
	return (off_t)result(__cortexos_syscall3(N_SYS_LSEEK, fd, offset, whence));
}

int fork(void)
{
	return (int)result(__cortexos_syscall0(N_SYS_FORK));
}

int execve(const char *path, char *const argv[], char *const envp[])
{
	return (int)result(__cortexos_syscall3(N_SYS_EXEC, path, argv, envp));
}

int wait(int *status)
{
	return (int)result(__cortexos_syscall1(N_SYS_WAIT, status));
}

int waitpid(pid_t pid, int *status, int options)
{
	return (int)result(__cortexos_syscall3(N_SYS_WAITPID, pid, status, options));
}

int getpid(void)
{
	return (int)result(__cortexos_syscall0(N_SYS_GETPID));
}

void _exit(int status)
{
	(void)__cortexos_syscall1(N_SYS_EXIT, status);
	for (;;)
		__asm__ volatile("hlt");
}

void *sbrk(intptr_t increment)
{
	long value = result(__cortexos_syscall1(N_SYS_SBRK, increment));
	return value < 0 ? (void *)-1 : (void *)value;
}

char *getcwd(char *buffer, size_t size)
{
	long value = result(__cortexos_syscall2(N_SYS_GETCWD, buffer, size));
	return value < 0 ? (char *)0 : buffer;
}

int chdir(const char *path)
{
	return (int)result(__cortexos_syscall1(N_SYS_CHDIR, path));
}

int stat(const char *path, struct stat *buffer)
{
	return (int)result(__cortexos_syscall2(N_SYS_STAT, path, buffer));
}

