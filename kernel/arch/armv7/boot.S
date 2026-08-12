.section .text.boot
.syntax unified
.cpu cortex-a15
.arm
.global _start
.type _start, %function

_start:
	cpsid if
	ldr sp, =__stack_top

	/* Install the ARMv7 vector table before entering C. */
	ldr r1, =armv7_vectors
	mcr p15, 0, r1, c12, c0, 0
	isb

	mov r4, r0
	mov r0, r4
	bl armv7_early_main

1:
	wfi
	b 1b

.align 5
.global armv7_vectors
.type armv7_vectors, %function
armv7_vectors:
	b armv7_exception_0
	b armv7_exception_1
	b armv7_exception_2
	b armv7_exception_3
	b armv7_exception_4
	b armv7_exception_5
	b armv7_exception_6
	b armv7_exception_7

.macro EXCEPTION_ENTRY number
.global armv7_exception_\number
.type armv7_exception_\number, %function
armv7_exception_\number:
	mov r0, #\number
	b armv7_exception_common
.endm

EXCEPTION_ENTRY 0
EXCEPTION_ENTRY 1
EXCEPTION_ENTRY 2
EXCEPTION_ENTRY 3
EXCEPTION_ENTRY 4
EXCEPTION_ENTRY 5
EXCEPTION_ENTRY 6
EXCEPTION_ENTRY 7

.global armv7_exception_common
.type armv7_exception_common, %function
armv7_exception_common:
	bl armv7_exception_c

2:
	wfi
	b 2b

.size _start, . - _start
