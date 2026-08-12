#include "gic.h"

#define GICD_CTLR           0x0000
#define GICD_ISENABLER0     0x0100
#define GICD_IPRIORITYR0    0x0400
#define GICD_ITARGETSR0     0x0800
#define GICC_CTLR           0x0000
#define GICC_PMR            0x0004
#define GICC_IAR            0x000c
#define GICC_EOIR           0x0010
#define TIMER_PPI           30U

static uint64_t gicd_base;
static uint64_t gicc_base;
static volatile uint64_t ticks;

static inline void write_cntp_tval(uint64_t value)
{
	__asm__ volatile("msr CNTP_TVAL_EL0, %0" : : "r"(value) : "memory");
}

static inline void write_cntp_ctl(uint64_t value)
{
	__asm__ volatile("msr CNTP_CTL_EL0, %0" : : "r"(value) : "memory");
}

static inline uint64_t read_cntfrq(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(value));
	return value;
}

static inline void mmio_write32(uint64_t address, uint32_t value)
{
	*(volatile uint32_t *)(uintptr_t)address = value;
}

static inline uint32_t mmio_read32(uint64_t address)
{
	return *(volatile uint32_t *)(uintptr_t)address;
}

static void timer_program(void)
{
	uint64_t frequency = read_cntfrq();
	if (!frequency)
		frequency = 1000000;
	write_cntp_tval(frequency / 10);
	write_cntp_ctl(1);
}

int aarch64_gic_init(uint64_t distributor, uint64_t cpu_interface)
{
	if (!distributor || !cpu_interface)
		return -1;
	gicd_base = distributor;
	gicc_base = cpu_interface;

	/* Configure the ARM GICv2 MMIO interface used by QEMU virt in CI. */
	mmio_write32(gicd_base + GICD_CTLR, 0);
	*(volatile uint8_t *)(uintptr_t)(gicd_base + GICD_IPRIORITYR0 + TIMER_PPI) = 0x80;
	*(volatile uint8_t *)(uintptr_t)(gicd_base + GICD_ITARGETSR0 + TIMER_PPI) = 1;
	mmio_write32(gicd_base + GICD_ISENABLER0, 1U << TIMER_PPI);
	mmio_write32(gicd_base + GICD_CTLR, 1);

	mmio_write32(gicc_base + GICC_PMR, 0xff);
	mmio_write32(gicc_base + GICC_CTLR, 1);
	__asm__ volatile("dsb sy\n\tisb" ::: "memory");

	timer_program();
	return 0;
}

void aarch64_irq_c(uint64_t interrupt_id)
{
	if (interrupt_id == TIMER_PPI) {
		ticks++;
		timer_program();
	}
}

uint64_t aarch64_ticks(void)
{
	return ticks;
}

uint64_t aarch64_read_irq(void)
{
	return mmio_read32(gicc_base + GICC_IAR);
}

void aarch64_end_irq(uint64_t interrupt_id)
{
	/* 1023 is the GIC spurious interrupt ID and must not be EOI'd. */
	if (interrupt_id < 1020)
		mmio_write32(gicc_base + GICC_EOIR, (uint32_t)interrupt_id);
}
