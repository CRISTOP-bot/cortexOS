#include "boot_memory.h"
#include "kstring.h"
#include "pmm.h"

void reserve_boot_modules(unsigned long mbi_addr)
{
    if (!mbi_addr)
        return;

    struct multiboot_info *mbi =
        (struct multiboot_info *)(uintptr_t)mbi_addr;
    pmm_reserve_range(mbi_addr, sizeof(*mbi));
    if (!(mbi->flags & 0x8))
        return;

    struct multiboot_module *mods =
        (struct multiboot_module *)(uintptr_t)mbi->mods_addr;
    pmm_reserve_range(mbi->mods_addr,
                      mbi->mods_count * sizeof(struct multiboot_module));
    for (unsigned long i = 0; i < mbi->mods_count; ++i) {
        const char *name =
            (const char *)(uintptr_t)mods[i].cmdline;
        if (name)
            pmm_reserve_range(mods[i].cmdline, kstrlen(name) + 1);
        if (name && kstrstr(name, "rootfs") &&
            mods[i].mod_end > mods[i].mod_start)
            pmm_reserve_range(mods[i].mod_start,
                              mods[i].mod_end - mods[i].mod_start);
    }
}
