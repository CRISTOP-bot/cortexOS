.text

/* struct process starts with 0x50 bytes before struct process_context. */
.equ CTX_RAX,    0x50
.equ CTX_RBX,    0x58
.equ CTX_RCX,    0x60
.equ CTX_RDX,    0x68
.equ CTX_RSI,    0x70
.equ CTX_RDI,    0x78
.equ CTX_RBP,    0x80
.equ CTX_RSP,    0x88
.equ CTX_R8,     0x90
.equ CTX_R9,     0x98
.equ CTX_R10,    0xa0
.equ CTX_R11,    0xa8
.equ CTX_R12,    0xb0
.equ CTX_R13,    0xb8
.equ CTX_R14,    0xc0
.equ CTX_R15,    0xc8
.equ CTX_RIP,    0xd0
.equ CTX_RFLAGS, 0xd8
.equ CTX_CR3,    0xe0
.equ CTX_CS,     0xe8
.equ CTX_SS,     0xf0

.global context_switch_to
.type context_switch_to, @function
context_switch_to:
    /* The caller is on the old kernel stack.  Build a fresh iret frame there
       and restore every general register from the selected process. */
    mov %rdi, %r10
    sub $40, %rsp
    mov CTX_RIP(%r10), %rax
    mov %rax, 0(%rsp)
    mov CTX_CS(%r10), %rax
    mov %rax, 8(%rsp)
    mov CTX_RFLAGS(%r10), %rax
    mov %rax, 16(%rsp)
    mov CTX_RSP(%r10), %rax
    mov %rax, 24(%rsp)
    mov CTX_SS(%r10), %rax
    mov %rax, 32(%rsp)

    mov CTX_CR3(%r10), %rax
    mov %rax, %cr3

    mov CTX_RAX(%r10), %rax
    mov CTX_RBX(%r10), %rbx
    mov CTX_RCX(%r10), %rcx
    mov CTX_RDX(%r10), %rdx
    mov CTX_RSI(%r10), %rsi
    mov CTX_RDI(%r10), %rdi
    mov CTX_RBP(%r10), %rbp
    mov CTX_R8(%r10), %r8
    mov CTX_R9(%r10), %r9
    mov CTX_R11(%r10), %r11
    mov CTX_R10(%r10), %r10
    mov CTX_R12(%r10), %r12
    mov CTX_R13(%r10), %r13
    mov CTX_R14(%r10), %r14
    mov CTX_R15(%r10), %r15
    iretq

.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    mov %rsp, %rdi
    call process_save_syscall_context
    /* process_save_syscall_context is allowed to use caller-saved registers;
       reload the ABI arguments from the saved frame before dispatch. */
    mov 72(%rsp), %rdi
    mov 80(%rsp), %rsi
    mov 88(%rsp), %rdx
    mov 40(%rsp), %r10
    mov 56(%rsp), %r8
    mov 112(%rsp), %rax
    call syscall_handler

    /* Keep the return value in r12 while recording it in the process model. */
    mov %rax, %r12
    mov %rax, %rdi
    call process_record_syscall_result
    mov %r12, 112(%rsp)

    /* Before returning to userspace, check for pending signals. The iret frame
     * is 40 bytes above the current RSP (after 16 general register pushes).
     * If a signal is delivered, the frame's RIP will be modified to jump to the
     * handler, and the frame's RSP will point to the sigreturn stub. */
    mov %rsp, %rdi
    mov 128(%rdi), %rsi  /* Load process* from the iret frame's CS slot... wait,
                          * that's not where process* is. We need to get it from
                          * process_current(). Let me rethink... */
    call process_current
    mov %rax, %rdi       /* First arg: process* */
    mov %rsp, %rsi
    add $128, %rsi       /* Second arg: iret frame pointer (rsp + 128) */
    call process_deliver_signal
    /* If process_deliver_signal returned 1, the iret frame was modified
     * and we'll jump to the signal handler on iretq. Otherwise, control
     * continues normally. */

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    iretq

.global tss_load
.type tss_load, @function
tss_load:
    mov %di, %ax
    ltr %ax
    ret
