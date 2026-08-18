#include "process.h"
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

static struct process processes[MAX_PROCESSES];
static int current_pid = -1;
static int next_pid = 1;
static bool scheduler_enabled = false;

extern void context_switch_to(struct process *next);

static struct process *find_process(int pid)
{
	for (int i = 0; i < MAX_PROCESSES; ++i)
		if (processes[i].pid == pid && processes[i].state != PROCESS_UNUSED)
			return &processes[i];
	return 0;
}

static struct process *alloc_process(void)
{
	for (int i = 0; i < MAX_PROCESSES; ++i)
		if (processes[i].state == PROCESS_UNUSED) return &processes[i];
	return 0;
}

static int alloc_pid(void) { return next_pid++; }

static bool map_user_range(uint64_t cr3, uint64_t start, uint64_t size,
			   unsigned int flags)
{
	for (uint64_t off = 0; off < size; off += PAGE_SIZE) {
		void *page = pmm_alloc_page();
		if (!page || !vmm_map_page_in(cr3, (uint64_t)(uintptr_t)page,
					     start + off, flags))
			return false;
	}
	return true;
}

void process_init(void)
{
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		processes[i].state = PROCESS_UNUSED;
		processes[i].pid = 0;
	}
	current_pid = -1;
	next_pid = 1;
	scheduler_enabled = false;
	console_print("[ OK ] Process manager initialized\n");
}

int process_create(const char *name, uint64_t entry, bool user)
{
	struct process *proc = alloc_process();
	if (!proc) return -1;
	kmemset(proc, 0, sizeof(*proc));
	proc->pid = alloc_pid();
	proc->parent_pid = current_pid;
	proc->state = PROCESS_READY;
	proc->sid = proc->pid;
	proc->pgid = proc->pid;
	proc->tty_pgid = proc->pid;
	if (tty_get_foreground() == 0) tty_set_foreground(proc->pgid);
	vfs_fd_table_init(&proc->fd_table);
	proc->user_code = USER_CODE_TOP;
	proc->user_stack = USER_STACK_TOP - USER_STACK_SIZE;
	proc->brk = USER_HEAP_BASE;
	proc->brk_limit = USER_HEAP_LIMIT;
	kstrcpy(proc->name, name ? name : "process", PROCESS_NAME_SIZE);

	proc->kernel_stack = (uint64_t)(uintptr_t)kmalloc_aligned(KERNEL_STACK_SIZE);
	if (!proc->kernel_stack) goto fail;
	proc->kernel_stack += KERNEL_STACK_SIZE;
	proc->ctx.cr3 = vmm_create_address_space();
	if (!proc->ctx.cr3) goto fail;
	if (user && !map_user_range(proc->ctx.cr3, proc->user_code, USER_CODE_SIZE,
					VMM_PRESENT | VMM_WRITE | VMM_USER)) goto fail;
	if (user && !map_user_range(proc->ctx.cr3, proc->user_stack, USER_STACK_SIZE,
					VMM_PRESENT | VMM_WRITE | VMM_USER)) goto fail;

	if (entry && user) {
		uint8_t *code_dst = (uint8_t *)(uintptr_t)proc->user_code;
		const uint8_t *code_src = (const uint8_t *)(uintptr_t)entry;
		uint64_t old_cr3 = vmm_current_address_space();
		vmm_switch_address_space(proc->ctx.cr3);
		for (uint64_t i = 0; i < 256; ++i) code_dst[i] = code_src[i];
		vmm_switch_address_space(old_cr3);
	}
	proc->ctx.rip = proc->user_code;
	proc->ctx.rsp = proc->user_stack + USER_STACK_SIZE - 8;
	proc->ctx.rflags = 0x202;
	proc->ctx.cs = user ? 0x1B : 0x08;
	proc->ctx.ss = user ? 0x23 : 0x10;
	return proc->pid;
fail:
	if (proc->ctx.cr3 && proc->ctx.cr3 != vmm_current_address_space())
		vmm_destroy_address_space(proc->ctx.cr3);
	proc->state = PROCESS_UNUSED;
	return -1;
}

struct process *process_current(void)
{ return current_pid < 0 ? 0 : find_process(current_pid); }

bool process_user_range(const void *address, size_t length, bool write)
{
	struct process *proc = process_current();
	return proc && vmm_user_range_valid(proc->ctx.cr3, (uint64_t)(uintptr_t)address,
			(uint64_t)length, write);
}

bool process_copy_user_string(char *dst, size_t dst_size, const char *src)
{
	if (!dst || dst_size == 0 || !src) return false;
	for (size_t i = 0; i + 1 < dst_size; ++i) {
		if (!process_user_range(src + i, 1, false)) return false;
		dst[i] = src[i];
		if (!dst[i]) return true;
	}
	dst[dst_size - 1] = '\0';
	return false;
}

/* Save a syscall's CPU frame.  syscall_entry pushes r15..rax, so frame[0]
 * is r15 and the CPU's iret frame starts at frame[15]. */
void process_save_syscall_context(uint64_t *frame)
{
	struct process *p = process_current();
	if (!p || !frame) return;
	p->ctx.r15 = frame[0]; p->ctx.r14 = frame[1]; p->ctx.r13 = frame[2];
	p->ctx.r12 = frame[3]; p->ctx.r11 = frame[4]; p->ctx.r10 = frame[5];
	p->ctx.r9 = frame[6]; p->ctx.r8 = frame[7]; p->ctx.rbp = frame[8];
	p->ctx.rdi = frame[9]; p->ctx.rsi = frame[10]; p->ctx.rdx = frame[11];
	p->ctx.rcx = frame[12]; p->ctx.rbx = frame[13]; p->ctx.rax = frame[14];
	p->ctx.rip = frame[15]; p->ctx.cs = frame[16]; p->ctx.rflags = frame[17];
	p->ctx.rsp = frame[18]; p->ctx.ss = frame[19];
	p->ctx.cr3 = vmm_current_address_space();
}

void process_record_syscall_result(int64_t result)
{
	struct process *p = process_current();
	if (p) p->ctx.rax = (uint64_t)result;
}

/* irq_common_stub has ds, eleven saved registers, num, err, then iret. */
void process_save_interrupt_context(uint64_t *frame)
{
	struct process *p = process_current();
	if (!p || !frame || (frame[15] & 3) != 3) return;
	p->ctx.r11 = frame[1]; p->ctx.r10 = frame[2]; p->ctx.r9 = frame[3];
	p->ctx.r8 = frame[4]; p->ctx.rdi = frame[5]; p->ctx.rsi = frame[6];
	p->ctx.rbp = frame[7]; p->ctx.rbx = frame[8]; p->ctx.rdx = frame[9];
	p->ctx.rcx = frame[10]; p->ctx.rax = frame[11]; p->ctx.rip = frame[14];
	p->ctx.cs = frame[15]; p->ctx.rflags = frame[16]; p->ctx.rsp = frame[17];
	p->ctx.ss = frame[18];
}

static bool clone_pages(uint64_t src_cr3, uint64_t dst_cr3, uint64_t start,
			uint64_t end)
{
	for (uint64_t page = start & ~(PAGE_SIZE - 1); page < end; page += PAGE_SIZE) {
		unsigned long src_phys = vmm_get_phys_in(src_cr3, page);
		if (!src_phys) continue;
		void *dst = pmm_alloc_page();
		if (!dst) return false;
		kmemcpy(dst, (const void *)(uintptr_t)(src_phys & ~(PAGE_SIZE - 1)), PAGE_SIZE);
		if (!vmm_map_page_in(dst_cr3, (uint64_t)(uintptr_t)dst, page,
					vmm_get_flags_in(src_cr3, page) | VMM_USER)) return false;
	}
	return true;
}

int process_fork(void)
{
	struct process *parent = process_current();
	struct process *child;
	if (!parent || parent->ctx.cs != 0x1B) return -1;
	child = alloc_process();
	if (!child) return -1;
	kmemset(child, 0, sizeof(*child));
	child->pid = alloc_pid(); child->parent_pid = parent->pid;
	child->state = PROCESS_READY; child->user_code = parent->user_code;
	child->sid = parent->sid; child->pgid = parent->pgid;
	child->tty_pgid = parent->tty_pgid;
	child->pending_signals = 0;
	child->blocked_signals = parent->blocked_signals;
	for (int si = 0; si < PROCESS_SIG_MAX; ++si)
		child->signal_handlers[si] = parent->signal_handlers[si];
	if (vfs_fd_table_clone(&child->fd_table, &parent->fd_table) < 0) {
		child->state = PROCESS_UNUSED;
		return -1;
	}
	child->user_stack = parent->user_stack; child->brk = parent->brk;
	child->brk_limit = parent->brk_limit; child->ctx = parent->ctx;
	child->ctx.rax = 0; kstrcpy(child->name, parent->name, PROCESS_NAME_SIZE);
	child->kernel_stack = (uint64_t)(uintptr_t)kmalloc_aligned(KERNEL_STACK_SIZE);
	child->ctx.cr3 = vmm_create_address_space();
	if (!child->kernel_stack || !child->ctx.cr3 ||
	    !clone_pages(parent->ctx.cr3, child->ctx.cr3, parent->user_code,
			  parent->user_code + USER_CODE_SIZE) ||
	    !clone_pages(parent->ctx.cr3, child->ctx.cr3, parent->user_stack,
			  parent->user_stack + USER_STACK_SIZE) ||
	    !clone_pages(parent->ctx.cr3, child->ctx.cr3, USER_HEAP_BASE,
			  parent->brk)) {
		if (child->ctx.cr3 && child->ctx.cr3 != vmm_current_address_space())
			vmm_destroy_address_space(child->ctx.cr3);
		vfs_fd_table_close_all(&child->fd_table);
		child->state = PROCESS_UNUSED;
		return -1;
	}
	child->kernel_stack += KERNEL_STACK_SIZE;
	return child->pid;
}

#define EXEC_MAX_ARGS 32
#define EXEC_MAX_ENVP 32
#define EXEC_STRING_MAX 128

static size_t bounded_strlen(const char *text, size_t limit)
{
	size_t n = 0;
	if (!text) return 0;
	while (n < limit && text[n]) ++n;
	return n;
}

static uint64_t build_stack(struct process *proc, const char *path,
			    const char *const *argv, const char *const *envp)
{
	const char *args[EXEC_MAX_ARGS], *envs[EXEC_MAX_ENVP];
	uint64_t arg_addr[EXEC_MAX_ARGS], env_addr[EXEC_MAX_ENVP];
	int argc = 0, envc = 0;
	uint64_t sp = proc->user_stack + USER_STACK_SIZE;
	if (argv) while (argc < EXEC_MAX_ARGS && argv[argc]) {
		args[argc] = argv[argc];
		++argc;
	}
	if (!argc) args[argc++] = path;
	if (envp) while (envc < EXEC_MAX_ENVP && envp[envc]) {
		envs[envc] = envp[envc];
		++envc;
	}
	for (int i = envc - 1; i >= 0; --i) {
		size_t n = bounded_strlen(envs[i], EXEC_STRING_MAX - 1) + 1;
		sp -= n; kmemcpy((void *)(uintptr_t)sp, envs[i], n); env_addr[i] = sp;
	}
	for (int i = argc - 1; i >= 0; --i) {
		size_t n = bounded_strlen(args[i], EXEC_STRING_MAX - 1) + 1;
		sp -= n; kmemcpy((void *)(uintptr_t)sp, args[i], n); arg_addr[i] = sp;
	}
	sp &= ~0xFULL;
	sp -= 8; *(uint64_t *)(uintptr_t)sp = 0;
	for (int i = envc - 1; i >= 0; --i) { sp -= 8; *(uint64_t *)(uintptr_t)sp = env_addr[i]; }
	sp -= 8; *(uint64_t *)(uintptr_t)sp = 0;
	for (int i = argc - 1; i >= 0; --i) { sp -= 8; *(uint64_t *)(uintptr_t)sp = arg_addr[i]; }
	sp -= 8; *(uint64_t *)(uintptr_t)sp = (uint64_t)argc;
	return sp >= proc->user_stack ? sp : 0;
}

static int exec_kernel(struct process *proc, const char *path,
			const char *const *argv, const char *const *envp)
{
	const void *data = vfs_read(path);
	size_t size = vfs_get_size(path);
	uint64_t entry, old_cr3 = proc->ctx.cr3, new_cr3;
	new_cr3 = vmm_create_address_space();
	if (!data || !size || !new_cr3 ||
	    !map_user_range(new_cr3, proc->user_code, USER_CODE_SIZE,
			    VMM_PRESENT | VMM_WRITE | VMM_USER) ||
	    !map_user_range(new_cr3, proc->user_stack, USER_STACK_SIZE,
			    VMM_PRESENT | VMM_WRITE | VMM_USER)) goto fail;
	vmm_switch_address_space(new_cr3);
	if (!elf_load_image(data, size, proc->user_code,
				proc->user_code + USER_CODE_SIZE, &entry)) goto fail_active;
	proc->ctx.cr3 = new_cr3;
	proc->brk = USER_HEAP_BASE;
	proc->ctx.rip = entry;
	proc->ctx.rsp = build_stack(proc, path, argv, envp);
	if (!proc->ctx.rsp) goto fail_active;
	vmm_switch_address_space(old_cr3);
	vmm_destroy_address_space(old_cr3);
	vmm_switch_address_space(new_cr3);
	return 0;
fail_active:
	vmm_switch_address_space(old_cr3);
fail:
	if (new_cr3 && new_cr3 != vmm_current_address_space()) vmm_destroy_address_space(new_cr3);
	return -1;
}

static bool copy_vector(char out[][EXEC_STRING_MAX], const char *const *vector,
			int max, int *count)
{
	if (!vector) { *count = 0; return true; }
	for (int i = 0; i < max; ++i) {
		const char *src;
		if (!process_user_range(vector + i, sizeof(src), false)) return false;
		src = vector[i];
		if (!src) { *count = i; return true; }
		if (!process_copy_user_string(out[i], EXEC_STRING_MAX, src)) return false;
	}
	return false;
}

int process_exec(const char *path, const char *const *argv,
			const char *const *envp)
{
	struct process *p = process_current();
	char path_copy[EXEC_STRING_MAX], args[EXEC_MAX_ARGS][EXEC_STRING_MAX];
	char envs[EXEC_MAX_ENVP][EXEC_STRING_MAX];
	const char *arg_ptrs[EXEC_MAX_ARGS], *env_ptrs[EXEC_MAX_ENVP];
	int argc, envc;
	if (!p || !process_copy_user_string(path_copy, sizeof(path_copy), path) ||
	    !copy_vector(args, argv, EXEC_MAX_ARGS, &argc) ||
	    !copy_vector(envs, envp, EXEC_MAX_ENVP, &envc)) return -1;
	for (int i = 0; i < argc; ++i) arg_ptrs[i] = args[i];
	for (int i = 0; i < envc; ++i) env_ptrs[i] = envs[i];
	arg_ptrs[argc] = 0; env_ptrs[envc] = 0;
	return exec_kernel(p, path_copy, arg_ptrs, env_ptrs);
}

int process_spawn_exec(const char *name, const char *path,
			const char *const *argv, const char *const *envp)
{
	int pid = process_create(name, 0, true);
	struct process *p;
	if (pid < 0) return -1;
	p = find_process(pid);
	if (!p || exec_kernel(p, path, argv, envp) < 0) {
		if (p) p->state = PROCESS_UNUSED;
		return -1;
	}
	return pid;
}

void process_exit(int code)
{
	struct process *p = process_current();
	if (!p) return;
	p->exit_code = code;
	vfs_fd_table_close_all(&p->fd_table);
	p->state = PROCESS_ZOMBIE;
	if (p->parent_pid > 0) {
		struct process *parent = find_process(p->parent_pid);
		if (parent) {
			parent->pending_signals |= PROCESS_SIGBIT(PROCESS_SIGCHLD);
			if (parent->state == PROCESS_BLOCKED) {
				parent->state = PROCESS_READY; parent->wait_exit = p->pid;
			}
		}
	}
	process_schedule();
}

int process_wait(int *status) { return process_waitpid(-1, status, 0); }

int process_waitpid(int pid, int *status, int options)
{
	struct process *p = process_current(); bool has_child = false;
	if (!p || pid == 0 || pid < -1) return -1;
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		if (processes[i].parent_pid != p->pid || (pid > 0 && processes[i].pid != pid)) continue;
		has_child = true;
		if (processes[i].state == PROCESS_ZOMBIE) {
			int child = processes[i].pid;
			if (status) *status = processes[i].exit_code;
			processes[i].state = PROCESS_UNUSED;
			p->pending_signals &= ~PROCESS_SIGBIT(PROCESS_SIGCHLD);
			return child;
		}
	}
	if (!has_child || (options & PROCESS_WNOHANG)) return has_child ? 0 : -1;
	p->state = PROCESS_BLOCKED; process_schedule();
	return process_waitpid(pid, status, PROCESS_WNOHANG);
}

int process_sbrk(intptr_t increment, uint64_t *old_break)
{
	struct process *p = process_current(); uint64_t old, next;
	if (!p || !old_break) return -1;
	old = p->brk;
	if (increment >= 0) {
		uint64_t amount = (uint64_t)increment;
		if (amount > p->brk_limit - old) return -1;
		next = old + amount;
		for (uint64_t page = (old + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
			 page < next; page += PAGE_SIZE) {
			void *mem = pmm_alloc_page();
			if (!mem || !vmm_map_page_in(p->ctx.cr3, (uint64_t)(uintptr_t)mem,
					page, VMM_PRESENT | VMM_WRITE | VMM_USER)) return -1;
		}
	} else {
		uint64_t amount = (uint64_t)(-(increment + 1)) + 1;
		if (amount > old - USER_HEAP_BASE) return -1;
		next = old - amount;
		for (uint64_t page = (next + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
			 page < old; page += PAGE_SIZE) {
			unsigned long phys = vmm_get_phys_in(p->ctx.cr3, page);
			vmm_unmap_page_in(p->ctx.cr3, page);
			if (phys) pmm_free_page((void *)(uintptr_t)(phys & ~(PAGE_SIZE - 1)));
		}
	}
	p->brk = next; *old_break = old; return 0;
}

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

void process_list(void)
{
	const char *names[] = { "unused", "running", "ready", "blocked", "zombie" };
	console_print_color("  PID  NAME                 STATE\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	for (int i = 0; i < MAX_PROCESSES; ++i) if (processes[i].state != PROCESS_UNUSED) {
		char buf[16]; int len;
		console_print("  "); kitoa(processes[i].pid, buf, sizeof(buf)); console_print(buf);
		len = kstrlen(buf); while (len++ < 5) console_print(" "); console_print("  ");
		console_print(processes[i].name); len = kstrlen(processes[i].name);
		while (len++ < 20) console_print(" ");
		console_print_color(names[processes[i].state], VGA_ATTR(VGA_LIGHT_GREY, VGA_BLACK)); console_print("\n");
	}
}

void process_start_scheduler(void) { scheduler_enabled = true; process_schedule(); }

void process_schedule(void)
{
	int start, best = -1; struct process *next, *prev;
	if (!scheduler_enabled) return;
	start = current_pid;
	for (int i = 0; i < MAX_PROCESSES; ++i) {
		int idx = (start + i + 1 + MAX_PROCESSES) % MAX_PROCESSES;
		if (processes[idx].state == PROCESS_READY) { best = idx; break; }
	}
	if (best < 0) return;
	next = &processes[best]; prev = process_current();
	if (prev == next) return;
	if (prev && prev->state == PROCESS_RUNNING) prev->state = PROCESS_READY;
	next->state = PROCESS_RUNNING; current_pid = next->pid;
	tss_set_rsp0(next->kernel_stack);
	context_switch_to(next);
}

void process_switch_to(struct process *next) { if (next) context_switch_to(next); }
int process_get_pid(void) { return current_pid; }
int keyboard_readline_user(char *buf, int maxlen) { return keyboard_readline(buf, maxlen); }
void process_yield(void) { process_schedule(); }
