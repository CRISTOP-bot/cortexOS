#ifndef NUCLEOS_AARCH64_GIC_H
#define NUCLEOS_AARCH64_GIC_H

#include <stdint.h>

int aarch64_gic_init(uint64_t distributor, uint64_t cpu_interface);
void aarch64_irq_c(uint64_t interrupt_id);

#endif
