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
	VECTOR_SLOT 5
	VECTOR_SLOT 6
	VECTOR_SLOT 7
	/* Lower EL using AArch64: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 8
	VECTOR_SLOT 9
	VECTOR_SLOT 10
	VECTOR_SLOT 11
	/* Lower EL using AArch32: sync, IRQ, FIQ, SError. */
	VECTOR_SLOT 12
	VECTOR_SLOT 13
	VECTOR_SLOT 14
	VECTOR_SLOT 15

.global aarch64_exception_entry
.type aarch64_exception_entry, %function
aarch64_exception_entry:
	bl aarch64_exception_c

2:
	wfe
	b 2b

.size _start, . - _start
