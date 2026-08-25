#include "process_internal.h"
#include "memory.h"
#include "vmm.h"
#include "vfs.h"
#include "console.h"
#include "kstring.h"
#include "timer.h"
#include "keyboard.h"
#include "tty.h"
#include "tss.h"
#include "asm.h"
#include "elf.h"
#include <stdint.h>

static bool valid_signal(int sig)
{
	return sig > 0 && sig < PROCESS_SIG_MAX && sig != 32;
}

int process_kill(int pid, int sig)
{
	struct process *target;
	if (!valid_signal(sig) && sig != 0) return -1;
	target = find_process(pid);
	if (!target || target->state == PROCESS_ZOMBIE) return -1;
	if (!sig) return 0;
	target->pending_signals |= PROCESS_SIGBIT(sig);
	/* A handler is delivered by the user return path when that path exists;
	 * default fatal signals must not be left running indefinitely. */
	if ((sig == PROCESS_SIGKILL || sig == PROCESS_SIGTERM || sig == PROCESS_SIGHUP ||
		 sig == PROCESS_SIGINT) && target->signal_handlers[sig - 1] == 0) {
		target->exit_code = 128 + sig;
		vfs_fd_table_close_all(&target->fd_table);
		target->state = PROCESS_ZOMBIE;
		if (target->parent_pid > 0) {
			struct process *parent = find_process(target->parent_pid);
			if (parent) {
				parent->pending_signals |= PROCESS_SIGBIT(PROCESS_SIGCHLD);
				if (parent->state == PROCESS_BLOCKED) parent->state = PROCESS_READY;
			}
		}
	}
	return 0;
}

int process_sigaction(int sig, uintptr_t handler, unsigned long flags,
		unsigned long mask, uintptr_t *old_handler,
		unsigned long *old_flags, unsigned long *old_mask)
{
	struct process *p = process_current();
	if (!p || !valid_signal(sig) || sig == PROCESS_SIGKILL) return -1;
	if (old_handler) *old_handler = p->signal_handlers[sig - 1];
	if (old_flags) *old_flags = 0;
	if (old_mask) *old_mask = p->blocked_signals;
	/* SIG_IGN is represented by 1; SIG_DFL by 0.  Preserve the disposition
	 * and mask even though frame-based handler delivery is not enabled yet. */
	p->signal_handlers[sig - 1] = handler;
	p->blocked_signals = mask;
	(void)flags;
	if (handler == 1) p->pending_signals &= ~PROCESS_SIGBIT(sig);
	return 0;
}

int process_setsid(void)
{
	struct process *p = process_current();
	if (!p || p->pgid == p->pid) return -1;
	p->sid = p->pid; p->pgid = p->pid; p->tty_pgid = p->pid;
	return p->sid;
}

int process_getsid(int pid)
{
	struct process *p = pid == 0 ? process_current() : find_process(pid);
	return p ? p->sid : -1;
}

int process_setpgid(int pid, int pgid)
{
	struct process *self = process_current();
	struct process *p = pid == 0 ? self : find_process(pid);
	if (!self || !p || pgid < 0 || (p != self && p->parent_pid != self->pid)) return -1;
	if (pgid == 0) pgid = p->pid;
	p->pgid = pgid;
	return 0;
}

int process_getpgrp(void)
{
	struct process *p = process_current();
	return p ? p->pgid : -1;
}

int process_tty_get(void *data, size_t size)
{
	struct termios value;
	if (!process_current() || !data || size != sizeof(value) || tty_get_termios(&value) < 0) return -1;
	kmemcpy(data, &value, sizeof(value)); return 0;
}

int process_tty_set(const void *data, size_t size)
{
	if (!process_current() || !data || size != sizeof(struct termios)) return -1;
	return tty_set_termios((const struct termios *)data);
}

int process_tty_getpgrp(void)
{
	return process_current() ? tty_get_foreground() : -1;
}

int process_tty_setpgrp(int pgid)
{
	struct process *self = process_current();
	bool found = false;
	if (!self || pgid <= 0) return -1;
	for (int i = 0; i < MAX_PROCESSES; ++i)
		if (processes[i].state != PROCESS_UNUSED && processes[i].sid == self->sid && processes[i].pgid == pgid) { found = true; break; }
	if (!found) return -1;
	return tty_set_foreground(pgid);
}

void process_tty_signal(int signal)
{
	int pgid = tty_get_foreground();
	if (pgid <= 0) return;
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		struct process *target = &processes[i];
		if (target->state != PROCESS_UNUSED && target->pgid == pgid)
			(void)process_kill(target->pid, signal);
	}
}

/* Small sigreturn stub placed on the user stack: calls the sigreturn syscall
 * and should be unreachable (sigreturn never returns). 6 bytes total. */
static const uint8_t sigreturn_stub[] = {
	0xb8, 0x23, 0x00, 0x00, 0x00,  /* movl $N_SYS_SIGRETURN(35), %eax */
	0xcd, 0x80                      /* int $0x80 */
};

/* Deliver a pending signal to the current process if it's in userspace.
 * iret_frame is the kernel stack frame that context_switch_to would use
 * (rip, cs, rflags, rsp, ss — 5 values at the top of the kernel stack).
 *
 * On success (signal delivered), modifies the frame's rip to point to the
 * handler and the frame's rsp to have the stub and handler parameter set up.
 * Returns 1 if a signal was delivered, 0 if none pending/blocked.
 *
 * This function is called from syscall_entry after process_record_syscall_result,
 * before the iretq that would return to user. */
int process_deliver_signal(struct process *proc, uint64_t *iret_frame)
{
	int sig;
	uintptr_t handler;
	uint64_t user_stack, stub_addr;
	uint8_t *stub_mem;
	uint64_t old_cr3;

	if (!proc || proc->ctx.cs != 0x1B) return 0;  /* Not in userspace */

	/* Find the first pending signal that is not blocked. */
	for (sig = 1; sig < PROCESS_SIG_MAX; ++sig) {
		if ((proc->pending_signals & PROCESS_SIGBIT(sig)) &&
		    !(proc->blocked_signals & PROCESS_SIGBIT(sig)))
			break;
	}
	if (sig >= PROCESS_SIG_MAX) return 0;  /* No pending signals */

	handler = proc->signal_handlers[sig - 1];
	if (!handler) return 0;  /* SIG_DFL (0) — handler not installed */
	if (handler == 1) {  /* SIG_IGN */
		proc->pending_signals &= ~PROCESS_SIGBIT(sig);
		return 0;
	}

	/* Clear this signal from pending (one signal at a time). */
	proc->pending_signals &= ~PROCESS_SIGBIT(sig);

	/* Save the interrupted context for sigreturn to restore. */
	proc->saved_sigframe.ctx = proc->ctx;
	proc->saved_sigframe.signal = sig;

	/* Prepare the user stack:
	 * - Decrement RSP by space for the stub and a return address.
	 * - Place the sigreturn stub at the new RSP.
	 * - Place the signal number in RDI (handler's first argument).
	 * - Set RIP to the handler.
	 * - When the handler does RET, it will jump to the stub.
	 */

	user_stack = proc->ctx.rsp - 16;  /* Space for stub (8) + alignment (8) */
	stub_addr = user_stack;

	/* Switch to the process's address space to write the stub. */
	old_cr3 = vmm_current_address_space();
	vmm_switch_address_space(proc->ctx.cr3);

	/* Copy the stub to the user stack. */
	stub_mem = (uint8_t *)(uintptr_t)stub_addr;
	kmemcpy(stub_mem, sigreturn_stub, sizeof(sigreturn_stub));

	/* Restore kernel address space. */
	vmm_switch_address_space(old_cr3);

	/* Modify the iret frame to jump to the handler.
	 * The frame layout (5 uint64_t values) is: rip, cs, rflags, rsp, ss.
	 * We only modify rip, rsp, and rdi. */
	iret_frame[0] = handler;         /* rip — jump to handler */
	iret_frame[3] = stub_addr;       /* rsp — point to our stub for return */
	proc->ctx.rdi = sig;             /* rdi — pass signal number as argument */

	return 1;  /* Signal delivered. */
}

/* Sigreturn syscall: restore the interrupted context saved by process_deliver_signal.
 * Does not return; transfers control back to the interrupted code. */
int process_sigreturn(void)
{
	struct process *p = process_current();
	if (!p) return -1;

	/* Restore the entire context. The caller will see this return value
	 * in RAX, but the context restore overrides everything anyway. */
	p->ctx = p->saved_sigframe.ctx;
	return 0;
}

