#include <stdint.h>

#define PL011_BASE 0x09000000UL
#define FALLBACK_DTB 0x47f00000UL
#define UART_FR_TXFF (1U << 5)

static uintptr_t uart_base = PL011_BASE;
#define UART_DR (*(volatile uint32_t *)(uart_base + 0x00))
#define UART_FR (*(volatile uint32_t *)(uart_base + 0x18))

static inline uint32_t read_midr(void)
{
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r"(value));
	return value;
}

static inline uint32_t read_sctlr(void)
{
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(value));
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

static void uart_puthex(uint32_t value)
{
	static const char digits[] = "0123456789abcdef";
	int shift;

	uart_puts("0x");
	for (shift = 28; shift >= 0; shift -= 4)
		uart_putc(digits[(value >> shift) & 0xf]);
}

static int fdt_valid(uint32_t address)
{
	volatile const uint8_t *p = (volatile const uint8_t *)(uintptr_t)address;
	return address && p[0] == 0xd0 && p[1] == 0x0d &&
	       p[2] == 0xfe && p[3] == 0xed;
}

static uint32_t find_fdt_in_ram(void)
{
	uint32_t address;
	for (address = 0x40000000UL; address < 0x48000000UL; address += 0x1000) {
		if (fdt_valid(address))
			return address;
	}
	return 0;
}

void armv7_early_main(uint32_t dtb)
{
	uint32_t discovered;
	if (!fdt_valid(dtb)) {
		discovered = find_fdt_in_ram();
		dtb = discovered ? discovered : FALLBACK_DTB;
	}

	uart_puts("NucleOS ARMv7 early boot\n");
	uart_puts("DTB: ");
	uart_puthex(dtb);
	uart_puts("\nMIDR: ");
	uart_puthex(read_midr());
	uart_puts("\nSCTLR: ");
	uart_puthex(read_sctlr());
	uart_puts("\nVector table installed in VBAR\n");
	if (fdt_valid(dtb))
		uart_puts("FDT: valid\n");
	else
		uart_puts("FDT: invalid or unavailable\n");
	uart_puts("Next: FDT parser, MMU, GIC and Generic Timer\n");
}

__attribute__((noreturn)) void armv7_exception_c(uint32_t vector)
{
	uart_puts("\nNucleOS ARMv7 exception\nVector: ");
	uart_puthex(vector);
	uart_puts("\n");
	for (;;)
		__asm__ volatile("wfi");
}
