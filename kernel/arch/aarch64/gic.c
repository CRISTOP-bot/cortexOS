#include "gic.h"

#define GICR_WAKER         0x14
#define GICR_IGROUPR0      0x80
#define GICR_ISENABLER0    0x100
#define GICR_IPRIORITYR0   0x400
#define GICD_CTLR          0x0000
#define TIMER_PPI          30U

static uint64_t gicr_base;
static volatile uint64_t ticks;

static inline void write_icc_sre(uint64_t value)
{
	__asm__ volatile("msr ICC_SRE_EL1, %0" : : "r"(value) : "memory");
}

static inline void write_icc_pmr(uint64_t value)
{
	__asm__ volatile("msr ICC_PMR_EL1, %0" : : "r"(value) : "memory");
}

static inline void write_icc_igrpen1(uint64_t value)
{
	__asm__ volatile("msr ICC_IGRPEN1_EL1, %0" : : "r"(value) : "memory");
}

static inline uint64_t read_icc_iar1(void)
{
	uint64_t value;
	__asm__ volatile("mrs %0, ICC_IAR1_EL1" : "=r"(value));
	return value;
}

static inline void write_icc_eoir1(uint64_t value)
{
	__asm__ volatile("msr ICC_EOIR1_EL1, %0" : : "r"(value) : "memory");
}

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

int aarch64_gicv3_init(uint64_t distributor, uint64_t redistributor)
{
	uint32_t waker;

	if (!distributor || !redistributor)
		return -1;
	gicr_base = redistributor;

	/* Enable system-register access before using ICC_* registers. */
	write_icc_sre(1);
	__asm__ volatile("isb" ::: "memory");

	/* Wake the current redistributor. */
	waker = mmio_read32(gicr_base + GICR_WAKER);
	waker &= ~(1U << 1);
	mmio_write32(gicr_base + GICR_WAKER, waker);
	while (mmio_read32(gicr_base + GICR_WAKER) & (1U << 2))
		;

	/* Put the timer PPI in Group 1, give it a usable priority and enable it. */
	mmio_write32(gicr_base + GICR_IGROUPR0, 1U << TIMER_PPI);
	*(volatile uint8_t *)(uintptr_t)(gicr_base + GICR_IPRIORITYR0 + TIMER_PPI) = 0x80;
	mmio_write32(gicr_base + GICR_ISENABLER0, 1U << TIMER_PPI);

	/* Enable Group 1 interrupts at the distributor and CPU interface. */
	mmio_write32(distributor + GICD_CTLR, 1U << 1);
	write_icc_pmr(0xff);
	write_icc_igrpen1(1);
	__asm__ volatile("isb" ::: "memory");

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
	return read_icc_iar1();
}

void aarch64_end_irq(uint64_t interrupt_id)
{
	/* 1023 is the GIC spurious interrupt ID and must not be EOI'd. */
	if (interrupt_id < 1020)
		write_icc_eoir1(interrupt_id);
}
