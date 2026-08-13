#ifndef CORTEXOS_AARCH64_GIC_H
#define CORTEXOS_AARCH64_GIC_H

#include <stdint.h>

int aarch64_gic_init(uint64_t distributor, uint64_t cpu_interface);
void aarch64_irq_c(uint64_t interrupt_id);
uint64_t aarch64_ticks(void);
uint64_t aarch64_read_irq(void);
void aarch64_end_irq(uint64_t interrupt_id);

#endif
