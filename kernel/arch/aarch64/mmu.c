#include "mmu.h"
#include "pmm.h"

extern void aarch64_mmu_trace(uint64_t stage, uint64_t value);

#define PAGE_SIZE       4096UL
#define TABLE_DESC      0x3UL
#define BLOCK_DESC      0x1UL
#define AF              (1UL << 10)
#define SH_INNER        (3UL << 8)
#define ATTR_DEVICE     (1UL << 2)
#define UXN             (1UL << 54)
#define PXN             (1UL << 53)
#define AP_EL0_RW       (1UL << 6)
#define SCTLR_M         (1UL << 0)
#define SCTLR_C         (1UL << 2)
#define SCTLR_I         (1UL << 12)

/* Page-table pages come from the physical allocator, not static fallbacks. */
static uint64_t *l1_table;
static uint64_t *l2_table;

static inline void write_ttbr0(uint64_t value)
{
	__asm__ volatile("msr TTBR0_EL1, %0" : : "r"(value) : "memory");
}

static inline void write_tcr(uint64_t value)
{
	__asm__ volatile("msr TCR_EL1, %0" : : "r"(value) : "memory");
}

static inline void write_mair(uint64_t value)
{
	__asm__ volatile("msr MAIR_EL1, %0" : : "r"(value) : "memory");
}

static inline uint64_t read_sctlr(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, SCTLR_EL1" : "=r"(value));
	return value;
}

static inline void write_sctlr(uint64_t value)
{
	__asm__ volatile("msr SCTLR_EL1, %0" : : "r"(value) : "memory");
}

static void clear_tables(void)
{
	for (unsigned int i = 0; i < 512; i++) {
		l1_table[i] = 0;
		l2_table[i] = 0;
	}
}

int aarch64_mmu_init(uint64_t ram_base, uint64_t ram_size)
{
	uint64_t ram_block;
	uint64_t ram_limit;
	uint64_t tcr;
	uint64_t sctlr;

	if (ram_base != 0x40000000UL || ram_size < 0x04000000UL)
		return -1;

	l1_table = (uint64_t *)(uintptr_t)aarch64_pmm_alloc_page();
	l2_table = (uint64_t *)(uintptr_t)aarch64_pmm_alloc_page();
	aarch64_mmu_trace(1, (uint64_t)(uintptr_t)l1_table);
	aarch64_mmu_trace(2, (uint64_t)(uintptr_t)l2_table);
	if (!l1_table || !l2_table)
		return -1;
	clear_tables();

	/* The first 1 GiB is split into 2 MiB device blocks. This covers the
	 * MMIO ranges described by QEMU virt, including UART and GIC. */
	l1_table[0] = ((uint64_t)(uintptr_t)l2_table & ~0xfffUL) | TABLE_DESC;
	for (unsigned int i = 0; i < 512; i++)
		l2_table[i] = ((uint64_t)i << 21) | BLOCK_DESC | AF | ATTR_DEVICE | UXN | PXN;

	/* Map the first RAM GiB as normal memory. The early image is loaded at
	 * 0x40080000, so this block also contains the kernel and its stack. */
	ram_block = ram_base & ~0x3fffffffUL;
	ram_limit = ram_base + ram_size;
	if (ram_block != 0x40000000UL || ram_limit < ram_base)
		return -1;
	l1_table[1] = ram_block | BLOCK_DESC | AF | SH_INNER | AP_EL0_RW;

	/* 39-bit VA, 4 KiB granule, inner-shareable WBWA normal memory. */
	tcr = 25UL | (1UL << 8) | (1UL << 10) | (3UL << 12) | (5UL << 32);
	write_mair(0x00000000000004ffUL);
	/* T0SZ=25 selects a 39-bit VA space, whose root is level 1. */
	aarch64_mmu_trace(3, tcr);
	write_ttbr0((uint64_t)(uintptr_t)l1_table);
	write_tcr(tcr);
	aarch64_mmu_trace(4, 0);
	__asm__ volatile("dsb ish\n\ttlbi vmalle1\n\tdsb ish\n\tisb" ::: "memory");

	sctlr = read_sctlr();
	sctlr |= SCTLR_M | SCTLR_C | SCTLR_I;
	aarch64_mmu_trace(5, sctlr);
	write_sctlr(sctlr);
	__asm__ volatile("isb" ::: "memory");
	return 0;
}
