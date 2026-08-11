#include <stdint.h>

#define PL011_BASE 0x09000000UL
#define UART_DR    (*(volatile uint32_t *)(PL011_BASE + 0x00))
#define UART_FR    (*(volatile uint32_t *)(PL011_BASE + 0x18))
#define UART_FR_TXFF (1U << 5)

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
	uart_puts("NucleOS AArch64 early boot\n");
	uart_puts("EL: ");
	uart_puthex((read_currentel() >> 2) & 0x3);
	uart_puts("\nDTB: ");
	uart_puthex(dtb);
	uart_puts("\nCNTFRQ: ");
	uart_puthex(read_cntfrq());
	uart_puts("\nCNTPCT: ");
	uart_puthex(read_cntpct());
	uart_puts("\nException vectors installed at VBAR_EL1\n");
	uart_puts("Next: Device Tree, MMU, GIC and timer interrupts\n");
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
