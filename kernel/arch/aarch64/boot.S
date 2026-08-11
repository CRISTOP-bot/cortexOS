.section .text.boot
.global _start
.type _start, %function

/* QEMU virt enters the image in EL1 with x0 containing the DTB address. */
_start:
	msr daifset, #0xf
	mov x20, x0
	ldr x1, =__stack_top
	mov sp, x1

	/* Install the exception vector table before calling C code. */
	adrp x1, aarch64_vectors
	add x1, x1, :lo12:aarch64_vectors
	msr VBAR_EL1, x1
	isb

	mov x0, x20
	bl aarch64_early_main

1:
	wfe
	b 1b

/*
 * Each AArch64 vector slot is 128 bytes and the complete table is aligned to
 * 2048 bytes. The index is passed to the common diagnostic handler.
 */
.macro VECTOR_SLOT number
	mov x0, #\number
	b aarch64_exception_entry
	.space 120
.endm

.macro VECTOR_IRQ_SLOT
	b aarch64_irq_entry
	.space 124
.endm

.align 11
.global aarch64_vectors
.type aarch64_vectors, %function
aarch64_vectors:
	/* Current EL with SP0: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 0
	VECTOR_SLOT 1
	VECTOR_SLOT 2
	VECTOR_SLOT 3
	/* Current EL with SPx: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 4
	VECTOR_IRQ_SLOT
	VECTOR_SLOT 6
	VECTOR_SLOT 7
	/* Lower EL using AArch64: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 8
	VECTOR_IRQ_SLOT
	VECTOR_SLOT 10
	VECTOR_SLOT 11
	/* Lower EL using AArch32: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 12
	VECTOR_SLOT 13
	VECTOR_SLOT 14
	VECTOR_SLOT 15

.global aarch64_irq_entry
.type aarch64_irq_entry, %function
aarch64_irq_entry:
	/* Preserve the complete interrupted register set before entering C. */
	sub sp, sp, #256
	stp x0, x1, [sp, #0]
	stp x2, x3, [sp, #16]
	stp x4, x5, [sp, #32]
	stp x6, x7, [sp, #48]
	stp x8, x9, [sp, #64]
	stp x10, x11, [sp, #80]
	stp x12, x13, [sp, #96]
	stp x14, x15, [sp, #112]
	stp x16, x17, [sp, #128]
	stp x18, x19, [sp, #144]
	stp x20, x21, [sp, #160]
	stp x22, x23, [sp, #176]
	stp x24, x25, [sp, #192]
	stp x26, x27, [sp, #208]
	stp x28, x29, [sp, #224]
	str x30, [sp, #240]

	bl aarch64_read_irq
	mov x19, x0
	mov x0, x19
	bl aarch64_irq_c
	mov x0, x19
	bl aarch64_end_irq

	ldr x30, [sp, #240]
	ldp x28, x29, [sp, #224]
	ldp x26, x27, [sp, #208]
	ldp x24, x25, [sp, #192]
	ldp x22, x23, [sp, #176]
	ldp x20, x21, [sp, #160]
	ldp x18, x19, [sp, #144]
	ldp x16, x17, [sp, #128]
	ldp x14, x15, [sp, #112]
	ldp x12, x13, [sp, #96]
	ldp x10, x11, [sp, #80]
	ldp x8, x9, [sp, #64]
	ldp x6, x7, [sp, #48]
	ldp x4, x5, [sp, #32]
	ldp x2, x3, [sp, #16]
	ldp x0, x1, [sp, #0]
	add sp, sp, #256
	eret

.global aarch64_exception_entry
.type aarch64_exception_entry, %function
aarch64_exception_entry:
	bl aarch64_exception_c

2:
	wfe
	b 2b

.size _start, . - _start
