#include <stdarg.h>
#include <libc/errno.h>
#include <libc/fcntl.h>
#include <abi/nucleos_syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libc/unistd.h>
#include <abi/signal.h>
#include <abi/termios.h>

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


int kill(int pid, int sig)
{
	return (int)result(__cortexos_syscall2(N_SYS_KILL, pid, sig));
}
int sigaction(int sig, const struct sigaction *action, struct sigaction *old_action)
{
	return (int)result(__cortexos_syscall3(N_SYS_SIGACTION, sig, action, old_action));
}
int ioctl(int fd, unsigned long request, ...)
{
	va_list args; void *arg; long value;
	va_start(args, request); arg = va_arg(args, void *); va_end(args);
	value = __cortexos_syscall3(N_SYS_IOCTL, fd, request, arg);
	return (int)result(value);
}
int setsid(void) { return (int)result(__cortexos_syscall0(N_SYS_SETSID)); }
int getsid(int pid) { return (int)result(__cortexos_syscall1(N_SYS_GETSID, pid)); }
int setpgid(int pid, int pgid) { return (int)result(__cortexos_syscall2(N_SYS_SETPGID, pid, pgid)); }
int getpgrp(void) { return (int)result(__cortexos_syscall0(N_SYS_GETPGRP)); }
int mount(const char *source, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
	(void)data; return (int)result(__cortexos_syscall4(N_SYS_MOUNT, source, target, filesystem, flags));
}
int umount(const char *target) { return (int)result(__cortexos_syscall1(N_SYS_UMOUNT, target)); }

int tcgetattr(int fd, struct termios *termios_p)
{ return ioctl(fd, TCGETS, termios_p); }
int tcsetattr(int fd, int actions, const struct termios *termios_p)
{
	unsigned long request = actions == TCSADRAIN ? TCSETSW :
		actions == TCSAFLUSH ? TCSETSF : TCSETS;
	return ioctl(fd, request, (void *)termios_p);
}
int tcgetpgrp(int fd)
{ int pgrp = -1; return ioctl(fd, TIOCGPGRP, &pgrp) < 0 ? -1 : pgrp; }
int tcsetpgrp(int fd, int pgrp)
{ return ioctl(fd, TIOCSPGRP, &pgrp); }

int mkdir(const char *path, unsigned int mode)
{ return (int)result(__cortexos_syscall2(N_SYS_MKDIR, path, mode)); }
int unlink(const char *path)
{ return (int)result(__cortexos_syscall1(N_SYS_UNLINK, path)); }
int access(const char *path, int mode)
{ return (int)result(__cortexos_syscall2(N_SYS_ACCESS, path, mode)); }
int getuid(void) { return (int)result(__cortexos_syscall0(N_SYS_GETUID)); }
int geteuid(void) { return getuid(); }
int getgid(void) { return (int)result(__cortexos_syscall0(N_SYS_GETGID)); }
int getegid(void) { return getgid(); }

void sigreturn(void)
{
	/* sigreturn never returns to this function; the kernel restores the
	 * interrupted context completely, transferring control back to userspace
	 * at the point where the signal occurred. */
	__cortexos_syscall0(N_SYS_SIGRETURN);
	__builtin_unreachable();
}
