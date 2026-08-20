#include "syscall.h"
#include "console.h"
#include "process.h"
#include "vfs.h"
#include "timer.h"
#include "kstring.h"
#include "signal.h"
#include "termios.h"

typedef int64_t (*syscall_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	if (count == 0)
		return 0;
	if (!process_user_range((const void *)buf, (size_t)count, true))
		return -1;
	return vfs_read_fd((int)fd, (void *)buf, (size_t)count);
}

static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	if (count && !process_user_range((const void *)buf, (size_t)count, false))
		return -1;
	return vfs_write_fd((int)fd, (const void *)buf, (size_t)count);
}

static int64_t sys_open(uint64_t path, uint64_t flags, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a3; (void)a4; (void)a5;
	char path_copy[256];
	if (!process_copy_user_string(path_copy, sizeof(path_copy), (const char *)path))
		return -1;
	return vfs_open_fd(path_copy, (int)flags);
}

static int64_t sys_close(uint64_t fd, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	if (fd <= 2)
		return 0;
	return vfs_close_fd((int)fd);
}

static int64_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	process_exit((int)code);
	return 0;
}

static int64_t sys_fork(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	return process_fork();
}

static int64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp, uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	if (!path)
		return -1;
	return process_exec((const char *)path,
		(const char *const *)argv, (const char *const *)envp);
}

static int64_t sys_wait(uint64_t status, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	if (status && !process_user_range((const void *)status, sizeof(int), true))
		return -1;
	return process_wait((int *)status);
}

static int64_t sys_waitpid(uint64_t pid, uint64_t status, uint64_t options, uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	if (status && !process_user_range((const void *)status, sizeof(int), true))
		return -1;
	return process_waitpid((int)pid, (int *)status, (int)options);
}

static int64_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	struct process *p = process_current();
	return p ? p->pid : -1;
}

static int64_t sys_sbrk(uint64_t increment, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	uint64_t old;
	if (process_sbrk((intptr_t)increment, &old) < 0)
		return -1;
	return (int64_t)old;
}

static int64_t sys_getcwd(uint64_t buf, uint64_t size, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a3; (void)a4; (void)a5;
	if (!buf || size == 0 || !process_user_range((const void *)buf, (size_t)size, true))
		return -1;
	const char *cwd = vfs_pwd();
	size_t len = kstrlen(cwd);
	if (len >= size)
		return -1;
	kstrcpy((char *)buf, cwd, size);
	return 0;
}

static int64_t sys_chdir(uint64_t path, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	char path_copy[256];
	if (!process_copy_user_string(path_copy, sizeof(path_copy), (const char *)path))
		return -1;
	return vfs_cd(path_copy) ? 0 : -1;
}

static int64_t sys_ps(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	process_list();
	return 0;
}

static int64_t sys_ticks(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	return (int64_t)timer_get_ticks();
}

static int64_t sys_dup2(uint64_t old_fd, uint64_t new_fd, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a3; (void)a4; (void)a5;
	if (old_fd <= 2 && new_fd <= 2)
		return old_fd == new_fd ? (int64_t)new_fd : -1;
	return vfs_dup_fd((int)old_fd, (int)new_fd);
}

static int64_t sys_isatty(uint64_t fd, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;
	return vfs_isatty_fd((int)fd) ? 1 : 0;
}

static int64_t sys_pipe(uint64_t pipe_fds, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	int fds[2];
	(void)a2; (void)a3; (void)a4; (void)a5;
	if (!pipe_fds || !process_user_range((const void *)pipe_fds, sizeof(fds), true) ||
	    vfs_pipe(fds) < 0)
		return -1;
	((int *)pipe_fds)[0] = fds[0];
	((int *)pipe_fds)[1] = fds[1];
	return 0;
}

static int64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence, uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	return vfs_lseek_fd((int)fd, (long)offset, (int)whence);
}

/* A deliberately small, fixed-layout stat ABI. Keep this layout identical to
 * include/sys/stat.h until the user ABI grows a 64-bit time_t layer. */
struct cortexos_stat {
	uint64_t st_size;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
};

#define CORTEXOS_S_IFREG 0100000U
#define CORTEXOS_S_IFDIR 0040000U

static int64_t sys_stat(uint64_t path, uint64_t output, uint64_t a3, uint64_t a4, uint64_t a5)
{
	struct cortexos_stat *st = (struct cortexos_stat *)output;
	char path_copy[256];
	(void)a3; (void)a4; (void)a5;
	if (!st || !process_user_range((const void *)st, sizeof(*st), true) ||
	    !process_copy_user_string(path_copy, sizeof(path_copy), (const char *)path) ||
	    !vfs_exists(path_copy))
		return -1;
	st->st_size = vfs_get_size(path_copy);
	st->st_mode = vfs_is_dir(path_copy) ? CORTEXOS_S_IFDIR : CORTEXOS_S_IFREG;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	return 0;
}

#define CORTEXOS_F_GETFL 3
#define CORTEXOS_F_SETFL 4

static int64_t sys_fcntl(uint64_t fd, uint64_t command, uint64_t value, uint64_t a4, uint64_t a5)
{
	int flags;
	(void)a4; (void)a5;
	flags = vfs_get_fd_flags((int)fd);
	if (flags < 0)
		return -1;
	if (command == CORTEXOS_F_GETFL)
		return flags;
	if (command == CORTEXOS_F_SETFL)
		return vfs_set_fd_flags((int)fd, (int)value);
	return -1;
}

static int64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t a3, uint64_t a4, uint64_t a5)
{
	(void)a3; (void)a4; (void)a5;
	return process_kill((int)pid, (int)sig);
}

static int64_t sys_sigaction(uint64_t sig, uint64_t action, uint64_t old,
		uint64_t a4, uint64_t a5)
{
	struct sigaction in, previous;
	uintptr_t old_handler = 0;
	unsigned long old_flags = 0, old_mask = 0;
	(void)a4; (void)a5;
	if (action && !process_user_range((const void *)action, sizeof(in), false)) return -1;
	if (old && !process_user_range((const void *)old, sizeof(previous), true)) return -1;
	if (action) { kmemcpy(&in, (const void *)action, sizeof(in)); }
	if (process_sigaction((int)sig, action ? (uintptr_t)in.sa_handler : 0,
			action ? in.sa_flags : 0, action ? in.sa_mask : 0,
			&old_handler, &old_flags, &old_mask) < 0) return -1;
	if (old) {
		previous.sa_handler = (sighandler_t)old_handler;
		previous.sa_flags = old_flags; previous.sa_mask = old_mask;
		kmemcpy((void *)old, &previous, sizeof(previous));
	}
	return 0;
}

static int64_t sys_setsid(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return process_setsid(); }
static int64_t sys_getsid(uint64_t pid, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a2; (void)a3; (void)a4; (void)a5; return process_getsid((int)pid); }
static int64_t sys_setpgid(uint64_t pid, uint64_t pgid, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a3; (void)a4; (void)a5; return process_setpgid((int)pid, (int)pgid); }
static int64_t sys_getpgrp(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return process_getpgrp(); }

static int64_t sys_ioctl(uint64_t fd, uint64_t request, uint64_t arg,
		uint64_t a4, uint64_t a5)
{
	(void)a4; (void)a5;
	if (!vfs_isatty_fd((int)fd)) return -1;
	if (request == TCGETS) {
		if (!arg || !process_user_range((const void *)arg, sizeof(struct termios), true)) return -1;
		return process_tty_get((void *)arg, sizeof(struct termios));
	}
	if (request == TCSETS || request == TCSETSW || request == TCSETSF) {
		if (!arg || !process_user_range((const void *)arg, sizeof(struct termios), false)) return -1;
		return process_tty_set((const void *)arg, sizeof(struct termios));
	}
	if (request == TIOCGPGRP) {
		if (!arg || !process_user_range((const void *)arg, sizeof(int), true)) return -1;
		*(int *)arg = process_tty_getpgrp(); return 0;
	}
	if (request == TIOCSPGRP) {
		if (!arg || !process_user_range((const void *)arg, sizeof(int), false)) return -1;
		return process_tty_setpgrp(*(const int *)arg);
	}
	return -1;
}

static int64_t sys_mount(uint64_t source, uint64_t target, uint64_t fstype,
		uint64_t flags, uint64_t a5)
{
	char source_copy[128], target_copy[128], type_copy[32];
	(void)a5;
	if (!process_copy_user_string(source_copy, sizeof(source_copy), (const char *)source) ||
		!process_copy_user_string(target_copy, sizeof(target_copy), (const char *)target) ||
		!process_copy_user_string(type_copy, sizeof(type_copy), (const char *)fstype)) return -1;
	return vfs_mount(source_copy, target_copy, type_copy, (unsigned long)flags);
}
static int64_t sys_umount(uint64_t target, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	char target_copy[128]; (void)a2; (void)a3; (void)a4; (void)a5;
	if (!process_copy_user_string(target_copy, sizeof(target_copy), (const char *)target)) return -1;
	return vfs_umount(target_copy);
}

static int64_t sys_mkdir(uint64_t path, uint64_t mode, uint64_t a3, uint64_t a4, uint64_t a5)
{
	char copy[256]; (void)mode; (void)a3; (void)a4; (void)a5;
	if (!process_copy_user_string(copy, sizeof(copy), (const char *)path)) return -1;
	return vfs_mkdir(copy) ? 0 : -1;
}
static int64_t sys_unlink(uint64_t path, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	char copy[256]; (void)a2; (void)a3; (void)a4; (void)a5;
	if (!process_copy_user_string(copy, sizeof(copy), (const char *)path)) return -1;
	return vfs_remove(copy) ? 0 : -1;
}
static int64_t sys_access(uint64_t path, uint64_t mode, uint64_t a3, uint64_t a4, uint64_t a5)
{
	char copy[256]; (void)mode; (void)a3; (void)a4; (void)a5;
	if (!process_copy_user_string(copy, sizeof(copy), (const char *)path)) return -1;
	return vfs_exists(copy) ? 0 : -1;
}
static int64_t sys_getuid(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return 0; }
static int64_t sys_getgid(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return 0; }
static int64_t sys_sigreturn(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{ (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return process_sigreturn(); }

static syscall_fn syscall_table[SYSCALL_MAX] = {
	sys_read,
	sys_write,
	sys_open,
	sys_close,
	sys_exit,
	sys_fork,
	sys_exec,
	sys_wait,
	sys_getpid,
	sys_sbrk,
	sys_getcwd,
	sys_chdir,
	sys_ps,
	sys_ticks,
	sys_dup2,
	sys_isatty,
	sys_pipe,
	sys_lseek,
	sys_stat,
	sys_fcntl,
	sys_waitpid,
	sys_kill,
	sys_sigaction,
	sys_setsid,
	sys_getsid,
	sys_setpgid,
	sys_getpgrp,
	sys_ioctl,
	sys_mount,
	sys_umount,
	sys_mkdir,
	sys_unlink,
	sys_access,
	sys_getuid,
	sys_getgid,
	sys_sigreturn,
};

void syscall_init(void)
{
	console_print("[ OK ] Syscalls initialized (INT 0x80)\n");
}

int64_t syscall_handler(uint64_t rdi, uint64_t rsi, uint64_t rdx,
			uint64_t r10, uint64_t r8, uint64_t rax)
{
	syscall_fn fn;
	if (rax >= SYSCALL_MAX)
		return -1;
	fn = syscall_table[rax];
	if (!fn)
		return -1;
	return fn(rdi, rsi, rdx, r10, r8);
}

