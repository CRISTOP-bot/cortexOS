# CortexOS source inventory. This mirrors Linux-style subsystem ownership while
# the active build remains controlled by Makefile/KERNEL_SRC_DIRS.

obj-y += kernel/core/
obj-y += kernel/console/
obj-y += kernel/system/
obj-y += drivers/
obj-y += block/
obj-y += fs/
obj-y += ipc/
obj-y += mm/
obj-y += net/
obj-y += init/
obj-y += virt/

# Architecture code is selected by ARCH in Makefile.
obj-$(CONFIG_ARCH_X86_64) += arch/x86_64/
