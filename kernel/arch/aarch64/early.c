#include <stdint.h>

#include "fdt.h"
#include "gic.h"
#include "mmu.h"
#include "pmm.h"

extern char __kernel_start[];
extern char __kernel_end[];
extern char aarch64_user_program[];
extern void aarch64_enter_user(uint64_t entry, uint64_t stack) __attribute__((noreturn));

#define PL011_BASE 0x09000000UL
#define FALLBACK_DTB 0x47f00000UL
#define FALLBACK_GICD 0x08000000UL
#define FALLBACK_GICC 0x08010000UL
#define UART_FR_TXFF (1U << 5)

static uintptr_t uart_base = PL011_BASE;
#define UART_DR (*(volatile uint32_t *)(uart_base + 0x00))
#define UART_FR (*(volatile uint32_t *)(uart_base + 0x18))

static inline uint64_t read_currentel(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, CurrentEL" : "=r"(value));
	return value;
}

static inline uint64_t read_cntfrq(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(value));
	return value;
}

static inline uint64_t read_cntpct(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, CNTPCT_EL0" : "=r"(value));
	return value;
}

static inline uint64_t read_esr(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, ESR_EL1" : "=r"(value));
	return value;
}

static inline uint64_t read_elr(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, ELR_EL1" : "=r"(value));
	return value;
}

static inline uint64_t read_far(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, FAR_EL1" : "=r"(value));
	return value;
}

static void uart_putc(char c)
{
	while (UART_FR & UART_FR_TXFF)
		;
	UART_DR = (uint32_t)c;
}

static void uart_puts(const char *text)
{
	while (*text) {
		if (*text == '\n')
			uart_putc('\r');
		uart_putc(*text++);
	}
}

void aarch64_uart_write(const char *text, uint64_t length)
{
	uint64_t i;
	for (i = 0; i < length; i++) {
		if (text[i] == '\n')
			uart_putc('\r');
		uart_putc(text[i]);
	}
}

static void uart_puthex(uint64_t value)
{
	static const char digits[] = "0123456789abcdef";
	int shift;

	uart_puts("0x");
	for (shift = 60; shift >= 0; shift -= 4)
		uart_putc(digits[(value >> shift) & 0xf]);
}

void aarch64_early_main(uint64_t dtb)
{
	aarch64_fdt_info_t fdt;
	uint64_t user_stack;
	int mmu_ready = 0;

	uart_puts("NucleOS AArch64 early boot\n");
	uart_puts("EL: ");
	uart_puthex((read_currentel() >> 2) & 0x3);
	if (!dtb) {
		dtb = FALLBACK_DTB;
		uart_puts("\nDTB register empty; using QEMU fallback address");
	}
	uart_puts("\nDTB: ");
	uart_puthex(dtb);
	uart_puts("\nCNTFRQ: ");
	uart_puthex(read_cntfrq());
	uart_puts("\nCNTPCT: ");
	uart_puthex(read_cntpct());
	uart_puts("\nException vectors installed at VBAR_EL1\n");

	if (aarch64_fdt_parse(dtb, &fdt) != 0) {
		uart_puts("FDT: invalid or unavailable\n");
		return;
	}
	uart_puts("FDT: valid\nRAM base: ");
	uart_puthex(fdt.ram_base);
	uart_puts("\nRAM size: ");
	uart_puthex(fdt.ram_size);
	if (fdt.has_uart) {
		uart_base = (uintptr_t)fdt.uart_base;
		uart_puts("\nUART base: ");
		uart_puthex(fdt.uart_base);
	}
	if (!fdt.has_gic) {
		/* QEMU virt fallback for firmware versions that omit GIC
		 * compatibility data from the generated DTB. */
		fdt.gic_dist_base = FALLBACK_GICD;
		fdt.gic_redist_base = FALLBACK_GICC;
		fdt.has_gic = 1;
		uart_puts("\nGIC: using QEMU virt fallback addresses\n");
	}
	if (fdt.has_gic) {
		uart_puts("GICD base: ");
		uart_puthex(fdt.gic_dist_base);
		uart_puts("\nGICC base: ");
		uart_puthex(fdt.gic_redist_base);
	}
	uart_puts("\n");

	if (aarch64_pmm_init(fdt.ram_base, fdt.ram_size,
	                     (uint64_t)(uintptr_t)__kernel_start,
	                     (uint64_t)(uintptr_t)__kernel_end,
	                     dtb, fdt.dtb_size) != 0) {
		uart_puts("PMM: initialization failed\n");
		return;
	}
	uart_puts("PMM: initialized; free pages: ");
	uart_puthex(aarch64_pmm_free_pages());
	uart_puts("\n");

	uart_puts("MMU: initializing\n");
	if (fdt.has_memory && aarch64_mmu_init(fdt.ram_base, fdt.ram_size) == 0) {
		mmu_ready = 1;
		uart_puts("MMU: enabled with identity mappings\n");
	} else {
		uart_puts("MMU: initialization failed\n");
	}
	if (mmu_ready && fdt.has_gic &&
	    aarch64_gic_init(fdt.gic_dist_base, fdt.gic_redist_base) == 0) {
		uart_puts("GICv2: initialized; timer PPI enabled\n");
		__asm__ volatile("msr daifclr, #2\n\tisb" ::: "memory");
		uart_puts("IRQ: unmasked\n");
	} else {
		uart_puts("GIC/MMU: not enabled\n");
		return;
	}

	user_stack = aarch64_pmm_alloc_page();
	if (!user_stack) {
		uart_puts("EL0: user stack allocation failed\n");
		return;
	}
	uart_puts("EL0: entering user program; SVC enabled\n");
	aarch64_enter_user((uint64_t)(uintptr_t)aarch64_user_program,
	                   user_stack + AARCH64_PAGE_SIZE);
	for (;;) {
		__asm__ volatile("wfe");
	}
}

__attribute__((noreturn)) void aarch64_exception_c(uint64_t vector)
{
	uart_puts("\nNucleOS AArch64 exception\n");
	uart_puts("Vector: ");
	uart_puthex(vector);
	uart_puts("\nESR_EL1: ");
	uart_puthex(read_esr());
	uart_puts("\nELR_EL1: ");
	uart_puthex(read_elr());
	uart_puts("\nFAR_EL1: ");
	uart_puthex(read_far());
	uart_puts("\n");

	for (;;) {
		__asm__ volatile("wfe");
	}
}
