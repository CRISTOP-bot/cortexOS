#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "pmm.h"
#include "vfs.h"

#define PROCESS_SIG_MAX 32
#define PROCESS_SIGBIT(sig) (1U << ((sig) - 1))
#define PROCESS_SIGHUP 1
#define PROCESS_SIGINT 2
#define PROCESS_SIGKILL 9
#define PROCESS_SIGTERM 15
#define PROCESS_SIGCHLD 17

#define MAX_PROCESSES 32
#define PROCESS_NAME_SIZE 64
#define USER_STACK_SIZE (PAGE_SIZE * 4)
#define KERNEL_STACK_SIZE (PAGE_SIZE * 4)
#define USER_STACK_TOP 0x7FFFFFF000ULL
#define USER_CODE_TOP 0x400000ULL
#define USER_CODE_SIZE (PAGE_SIZE * 32)
#define USER_HEAP_BASE 0x10000000ULL
#define USER_HEAP_LIMIT 0x20000000ULL

/* The context is the complete user-visible register state needed to resume
 * after an interrupt or syscall. cr3 is the physical root of this process. */
enum process_state {
	PROCESS_UNUSED = 0,
	PROCESS_RUNNING,
	PROCESS_READY,
	PROCESS_BLOCKED,
	PROCESS_ZOMBIE,
};

struct process_context {
	uint64_t rax, rbx, rcx, rdx;
	uint64_t rsi, rdi, rbp, rsp;
	uint64_t r8, r9, r10, r11;
	uint64_t r12, r13, r14, r15;
	uint64_t rip;
	uint64_t rflags;
	uint64_t cr3;
	uint64_t cs, ss;
};

/* Saved signal context: stored in process when a signal handler is invoked,
 * so that sigreturn can restore the interrupted userspace state. */
struct cortexos_sigframe {
	struct process_context ctx;
	int signal;
};

struct process {
	int pid;
	int parent_pid;
	enum process_state state;
	char name[PROCESS_NAME_SIZE];
	struct process_context ctx;
	uint64_t kernel_stack;
	uint64_t user_stack;
	uint64_t user_code;
	uint64_t brk;
	uint64_t brk_limit;
	int exit_code;
	int wait_exit;
	/* POSIX process identity and signal state. */
	int sid;
	int pgid;
	int tty_pgid;
	uint32_t pending_signals;
	uint32_t blocked_signals;
	uintptr_t signal_handlers[PROCESS_SIG_MAX];
	/* Signal delivery frame: populated when a signal handler is invoked,
	 * consumed by sigreturn. Only one active signal delivery at a time. */
	struct cortexos_sigframe saved_sigframe;
	struct vfs_fd_table fd_table;
};

void process_init(void);
int process_create(const char *name, uint64_t entry, bool user);
int process_spawn_exec(const char *name, const char *path,
		       const char *const *argv, const char *const *envp);
void process_exit(int code);
int process_fork(void);
int process_exec(const char *path, const char *const *argv,
			 const char *const *envp);
#define PROCESS_WNOHANG 1
int process_wait(int *status);
int process_waitpid(int pid, int *status, int options);
int process_kill(int pid, int sig);
int process_sigaction(int sig, uintptr_t handler, unsigned long flags,
                      unsigned long mask, uintptr_t *old_handler,
                      unsigned long *old_flags, unsigned long *old_mask);
int process_setsid(void);
int process_getsid(int pid);
int process_setpgid(int pid, int pgid);
int process_getpgrp(void);
int process_tty_get(void *termios_data, size_t size);
int process_tty_set(const void *termios_data, size_t size);
int process_tty_getpgrp(void);
int process_tty_setpgrp(int pgid);
void process_tty_signal(int signal);
struct process *process_current(void);
void process_list(void);
void process_schedule(void);
void process_start_scheduler(void);
void process_switch_to(struct process *next);
int process_get_pid(void);

/* Called by the low-level entry stubs before changing address spaces. */
void process_save_syscall_context(uint64_t *frame);
void process_record_syscall_result(int64_t result);
void process_save_interrupt_context(uint64_t *frame);

/* Signal delivery: checks for pending signals and modifies the iret frame
 * to jump to the signal handler. Returns 1 if a signal was delivered,
 * 0 if no pending signals or they're blocked. The frame must be the kernel
 * stack iret frame that context_switch_to would use. */
int process_deliver_signal(struct process *proc, uint64_t *iret_frame);
int process_sigreturn(void);

bool process_user_range(const void *address, size_t length, bool write);
bool process_copy_user_string(char *dst, size_t dst_size, const char *src);
int process_sbrk(intptr_t increment, uint64_t *old_break);

int keyboard_readline_user(char *buf, int maxlen);

#endif
