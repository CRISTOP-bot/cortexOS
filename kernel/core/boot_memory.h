#ifndef CORTEXOS_BOOT_MEMORY_H
#define CORTEXOS_BOOT_MEMORY_H

#include <stdint.h>

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
};

struct multiboot_module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
};

void reserve_boot_modules(unsigned long mbi_addr);

#endif
