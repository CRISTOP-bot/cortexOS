# Target architecture. Only x86_64 is bootable today; other ports are
# explicitly rejected by check-arch until their architecture code is complete.
ARCH ?= x86_64
SUPPORTED_ARCHES = x86_64

ifeq ($(ARCH),x86_64)
	ARCH_SUPPORTED = yes
	CC  = gcc
	AS  = gcc
	LD  = ld
else ifeq ($(ARCH),i386)
	ARCH_SUPPORTED = no
	CC  = i686-elf-gcc
	AS  = i686-elf-gcc
	LD  = i686-elf-ld
else ifeq ($(ARCH),aarch64)
	ARCH_SUPPORTED = no
	CC  = aarch64-none-elf-gcc
	AS  = aarch64-none-elf-gcc
	LD  = aarch64-none-elf-ld
else ifeq ($(ARCH),armv7)
	ARCH_SUPPORTED = no
	CC  = arm-none-eabi-gcc
	AS  = arm-none-eabi-gcc
	LD  = arm-none-eabi-ld
else
	ARCH_SUPPORTED = no
	CC  = false
	AS  = false
	LD  = false
endif

# Project layout
BUILD_DIR   = build
DIST_DIR    = dist
ISO_DIR     = $(BUILD_DIR)/iso
KERNEL_DIR  = core/kernel
ARCH_DIR    = arch/$(ARCH)
KERNEL_SRC_DIRS = core/kernel/core core/kernel/apps core/kernel/console core/kernel/graphics core/kernel/system core/kernel/services hw/drivers/tty hw/block/ata hw/block/partition core/fs/core core/fs/crfs core/fs/elf core/fs/ext2 boot/core boot/openrc core/ipc/process core/ipc/syscall core/mm/physical core/mm/virtual core/mm/heap hw/net/core hw/drivers/console hw/drivers/input hw/drivers/interrupts hw/drivers/pci hw/drivers/serial core/lib core/lib/string
CONFIG_DIR  = project/config
GRUB_DIR    = $(CONFIG_DIR)/grub
ROOTFS_DIR  = userspace/rootfs
TOOLS_DIR   = project/tools

PYTHON      ?= python3
BASH_SRC_DIR    = project/third_party/bash
OPENRC_SRC_DIR  = project/third_party/openrc
OPENRC_COMMIT   = 04d75bc192486fee932e4e602bdfffc32a0d8b96
OPENRC_BIN_DIR   ?= $(BUILD_DIR)/openrc/bin
OPENRC_STAGE_DIR ?= $(ROOTFS_DIR)
FASTFETCH_SRC_DIR = project/third_party/fastfetch
FASTFETCH_COMMIT  = a0452b8323aaa9d3b5b6ded435ed6660cee2bbb9
DOOM_SRC_DIR      = project/third_party/doom
DOOM_COMMIT       = a77dfb96cb91780ca334d0d4cfd86957558007e0
DOOM_HOST_CC      ?= gcc
DOOM_PLATFORM_BUILD = $(BUILD_DIR)/doom-platform
DOOM_PLATFORM_TEST  = $(DOOM_PLATFORM_BUILD)/check

KERNEL      = $(BUILD_DIR)/kernel.bin
ROOTFS      = $(ISO_DIR)/boot/rootfs.bin
ISO_IMAGE   = $(DIST_DIR)/os.iso
QEMU        = qemu-system-x86_64
QEMU_AARCH64 ?= qemu-system-aarch64
LINKER      = $(KERNEL_DIR)/linker.ld
AARCH64_CC  ?= aarch64-linux-gnu-gcc
AARCH64_LD  ?= aarch64-linux-gnu-ld
AARCH64_BUILD = $(BUILD_DIR)/aarch64
AARCH64_EARLY = $(AARCH64_BUILD)/early.elf
AARCH64_DTB = $(AARCH64_BUILD)/virt.dtb
AARCH64_DTB_ADDRESS = 0x47f00000
AARCH64_OBJECTS = $(AARCH64_BUILD)/boot.o $(AARCH64_BUILD)/early.o \
	$(AARCH64_BUILD)/fdt.o $(AARCH64_BUILD)/mmu.o $(AARCH64_BUILD)/pmm.o \
	$(AARCH64_BUILD)/gic.o $(AARCH64_BUILD)/syscall.o $(AARCH64_BUILD)/user.o
ARMV7_CC ?= arm-linux-gnueabihf-gcc
ARMV7_LD ?= arm-linux-gnueabihf-ld
QEMU_ARMV7 ?= qemu-system-arm
ARMV7_BUILD = $(BUILD_DIR)/armv7
ARMV7_EARLY = $(ARMV7_BUILD)/early.elf
ARMV7_DTB = $(ARMV7_BUILD)/virt.dtb
ARMV7_DTB_ADDRESS = 0x47f00000

KERNEL_INCLUDES = $(foreach dir,$(KERNEL_SRC_DIRS),-I $(dir)) -I core/kernel/include -I userspace/include -I project/rust/include -I $(ARCH_DIR)
CFLAGS  = -ffreestanding -O2 -Wall -Wextra -m64 -nostdlib -std=c99 $(KERNEL_INCLUDES) -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -fno-strict-aliasing -mno-red-zone -mcmodel=kernel -fno-pic -fno-pie
ASFLAGS = -m64 -ffreestanding
LDFLAGS = -m elf_x86_64 -nostdlib

RUSTC       = rustc
RUST_TARGET = x86_64-unknown-linux-gnu
RUSTFLAGS   = -C no-redzone=yes -C code-model=kernel -C relocation-model=static
RUSTFLAGS  += -C panic=abort -C debuginfo=0 -C opt-level=2
RUST_SRC    = project/rust/kernel/rust_kernel.rs
RUST_OBJ    = $(BUILD_DIR)/rust_kernel.o

# Every top-level kernel subsystem owns its sources. Adding a C file to one of
# these directories automatically includes it in the x86_64 kernel.
KERNEL_SRCS = $(foreach dir,$(KERNEL_SRC_DIRS),$(wildcard $(dir)/*.c))
KERNEL_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ARCH_OBJS   = $(BUILD_DIR)/arch_asm_utils.o \
              $(BUILD_DIR)/arch_math_asm.o \
              $(BUILD_DIR)/arch_ctx_switch.o
OBJS        = $(KERNEL_OBJS) $(ARCH_OBJS) \
              $(BUILD_DIR)/boot_entry.o $(RUST_OBJ)

INSTALLER_FILES = project/tools/installer/__init__.py \
                  project/tools/installer/ui.py \
                  project/tools/installer/disk.py \
                  project/tools/installer/config.py \
                  project/tools/installer/install.py \
                  project/tools/installer/nucleos-install
LCP_FILES      = project/tools/lcp/lcp.py project/tools/lcp/main_repo.json

all: check-arch $(KERNEL)

check-arch:
ifeq ($(ARCH_SUPPORTED),yes)
	@echo "  Arquitectura seleccionada: $(ARCH)"
else
	$(error ARCH=$(ARCH) todavía no tiene un port arrancable; consulta project/Documentation/ARCHITECTURES.md)
endif

arch-list:
	@echo "Arquitecturas declaradas: x86_64 i386 aarch64 armv7"
	@echo "Kernel completo arrancable: x86_64"
	@echo "Etapas de boot independientes: aarch64 armv7"
	@echo "Preparadas para port completo: i386 armv7 aarch64"

$(BUILD_DIR):
	mkdir -p $@

$(DIST_DIR):
	mkdir -p $@

$(ISO_DIR)/boot/grub: $(GRUB_DIR)/grub.cfg $(GRUB_DIR)/theme.txt
	mkdir -p $@
	cp $(GRUB_DIR)/grub.cfg $(GRUB_DIR)/theme.txt $@/

$(BUILD_DIR)/boot_entry.o: $(ARCH_DIR)/boot.S | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/arch_%.o: $(ARCH_DIR)/%.S | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(RUST_OBJ): $(RUST_SRC) | $(BUILD_DIR)
	$(RUSTC) --target $(RUST_TARGET) --crate-type staticlib $(RUSTFLAGS) --emit obj -o $@ $(RUST_SRC)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) $(LINKER) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -T $(LINKER) -o $@ $(OBJS)

$(ISO_DIR)/installer: $(INSTALLER_FILES) $(LCP_FILES) | $(ISO_DIR)/boot/grub
	mkdir -p $@
	cp -r project/tools/installer/. $@/
	cp $(LCP_FILES) $@/
	rm -rf $@/__pycache__
	@echo "  Instalador y LCP copiados al ISO"

$(ROOTFS): $(TOOLS_DIR)/build/rootfs.py $(ROOTFS_DIR)/README.txt $(ROOTFS_DIR)/info.txt | $(ISO_DIR)/boot/grub
	$(PYTHON) $(TOOLS_DIR)/build/rootfs.py $(ROOTFS_DIR) $(ROOTFS)

iso: all $(ROOTFS) $(ISO_DIR)/installer
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin

# Creates the final artifact in dist/ and keeps generated files out of source.
echo-iso: iso | $(DIST_DIR)
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)
	@echo "  ISO generada: $(ISO_IMAGE)"

run: echo-iso
	$(QEMU) -cdrom $(ISO_IMAGE) -m 512M

installer:
	@echo ""
	@echo "  Para ejecutar el instalador DESDE EL ISO/USB:"
	@echo "    1. Monta el ISO/USB:  mount /dev/sdX1 /mnt"
	@echo "    2. Ejecuta:           sudo python /mnt/installer/nucleos-install"
	@echo ""
	@echo "  Para ejecutar el instalador LOCALMENTE:"
	@echo "    sudo project/tools/installer/nucleos-install"
	@echo ""
	@echo "  Debes compilar primero: make echo-iso"
	@echo ""

installer-usb:
	@echo ""
	@echo "  Crear USB booteable con instalador:"
	@echo "    sudo bash project/tools/media/make-usb.sh /dev/sdX"
	@echo ""
	@echo "  ADVERTENCIA: Esto BORRA todos los datos del dispositivo"
	@echo ""

user-libc:
	$(MAKE) -C userspace/usr CC="$(CC)"

user-test-hello:
	$(MAKE) -C userspace/usr CC="$(CC)" test-hello

user-test-posix:
	$(MAKE) -C userspace/usr CC="$(CC)" test-posix

# Doom source is kept as an upstream GPLv2 submodule. This check intentionally
# does not download an IWAD or attempt to call the incomplete engine port.
doom-source:
	@test -f $(DOOM_SRC_DIR)/LICENSE.TXT
	@test -f $(DOOM_SRC_DIR)/linuxdoom-1.10/d_main.c
	@test "$$(git -C $(DOOM_SRC_DIR) rev-parse HEAD)" = "$(DOOM_COMMIT)"
	@grep -qi 'GNU GENERAL PUBLIC LICENSE' $(DOOM_SRC_DIR)/LICENSE.TXT
	@echo "  Original Doom GPLv2 source available at $(DOOM_SRC_DIR) ($(DOOM_COMMIT))"
	@echo "  IWAD/PWAD data is deliberately not present; supply compatible data separately"

$(DOOM_PLATFORM_BUILD):
	mkdir -p $@

$(DOOM_PLATFORM_TEST): userspace/usr/doom/cortexos_platform.c userspace/usr/doom/cortexos_platform.h project/tools/validate/doom_platform_test.c | $(DOOM_PLATFORM_BUILD)
	$(DOOM_HOST_CC) -std=c99 -Wall -Wextra -Werror -Iuserspace/usr/doom \
		userspace/usr/doom/cortexos_platform.c project/tools/validate/doom_platform_test.c -o $@

doom-platform-check: $(DOOM_PLATFORM_TEST)
	$(DOOM_PLATFORM_TEST)

TTY_LINE_TEST = $(BUILD_DIR)/tty-line-test
$(TTY_LINE_TEST): hw/drivers/tty/tty_line.c hw/drivers/tty/tty_line.h userspace/include/termios.h project/tools/validate/tty_line_test.c | $(BUILD_DIR)
	$(DOOM_HOST_CC) -std=c99 -Wall -Wextra -Ihw/drivers/tty -Iuserspace/include \
		hw/drivers/tty/tty_line.c project/tools/validate/tty_line_test.c -o $@

tty-check: $(TTY_LINE_TEST)
	$(TTY_LINE_TEST)

# The combined check validates only source provenance and the platform boundary;
# it is not a playable-game target.
doom-check: doom-source doom-platform-check

# First independently buildable AArch64 stage. This does not yet build the
# x86_64-oriented top-level kernel subsystems; it validates the ARM64 boot and UART path.
aarch64-early: $(AARCH64_EARLY)
	@echo "  AArch64 early image: $(AARCH64_EARLY)"

$(AARCH64_BUILD):
	mkdir -p $@

$(AARCH64_BUILD)/boot.o: arch/aarch64/boot.S | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -march=armv8-a $< -o $@

$(AARCH64_BUILD)/early.o: arch/aarch64/early.c arch/aarch64/fdt.h arch/aarch64/mmu.h arch/aarch64/gic.h arch/aarch64/pmm.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_BUILD)/fdt.o: arch/aarch64/fdt.c arch/aarch64/fdt.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_BUILD)/mmu.o: arch/aarch64/mmu.c arch/aarch64/mmu.h arch/aarch64/pmm.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_BUILD)/pmm.o: arch/aarch64/pmm.c arch/aarch64/pmm.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_BUILD)/syscall.o: arch/aarch64/syscall.c arch/aarch64/gic.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_BUILD)/user.o: arch/aarch64/user.S | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -march=armv8-a $< -o $@

$(AARCH64_BUILD)/gic.o: arch/aarch64/gic.c arch/aarch64/gic.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -Iarch/aarch64 $< -o $@

$(AARCH64_EARLY): $(AARCH64_OBJECTS) arch/aarch64/linker.ld
	$(AARCH64_LD) -nostdlib -T arch/aarch64/linker.ld -o $@ $(AARCH64_OBJECTS)

# QEMU virt exposes the PL011 UART at 0x09000000. QEMU does not always
# provide its generated DTB in x0 when loading an ELF with -kernel, so create
# the machine DTB explicitly and load it at the documented fallback address.
aarch64-run: aarch64-early
	$(QEMU_AARCH64) -machine virt,gic-version=2,dumpdtb=$(AARCH64_DTB) -cpu cortex-a57 -m 128M -display none
	$(QEMU_AARCH64) -machine virt,gic-version=2 -cpu cortex-a57 -m 128M -nographic -monitor none -serial stdio -no-reboot -device loader,file=$(AARCH64_DTB),addr=$(AARCH64_DTB_ADDRESS) -kernel $(AARCH64_EARLY)

# First independently buildable ARMv7 stage. This is separate from the
# AArch64 image and does not yet build top-level kernel subsystems.
armv7-early: $(ARMV7_EARLY)
	@echo "  ARMv7 early image: $(ARMV7_EARLY)"

$(ARMV7_BUILD):
	mkdir -p $@

$(ARMV7_BUILD)/boot.o: arch/armv7/boot.S | $(ARMV7_BUILD)
	$(ARMV7_CC) -c -ffreestanding -nostdlib -marm -march=armv7-a -mfloat-abi=soft $< -o $@

$(ARMV7_BUILD)/early.o: arch/armv7/early.c | $(ARMV7_BUILD)
	$(ARMV7_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -marm -march=armv7-a -mfloat-abi=soft $< -o $@

$(ARMV7_EARLY): $(ARMV7_BUILD)/boot.o $(ARMV7_BUILD)/early.o arch/armv7/linker.ld
	$(ARMV7_LD) -nostdlib -T arch/armv7/linker.ld -o $@ $(ARMV7_BUILD)/boot.o $(ARMV7_BUILD)/early.o

armv7-run: armv7-early
	$(QEMU_ARMV7) -machine virt,dumpdtb=$(ARMV7_DTB) -cpu cortex-a15 -m 128M -display none
	$(QEMU_ARMV7) -machine virt -cpu cortex-a15 -m 128M -nographic -monitor none -serial stdio -no-reboot -dtb $(ARMV7_DTB) -device loader,file=$(ARMV7_DTB),addr=$(ARMV7_DTB_ADDRESS) -kernel $(ARMV7_EARLY)

openrc-source:
	@test -f $(OPENRC_SRC_DIR)/meson.build
	@test -d $(OPENRC_SRC_DIR)/src
	@test -f $(OPENRC_SRC_DIR)/src/openrc-init/openrc-init.c
	@test -f $(OPENRC_SRC_DIR)/src/rc-service/rc-service.c
	@test "$$(git -C $(OPENRC_SRC_DIR) rev-parse HEAD)" = "$(OPENRC_COMMIT)"
	@echo "  Fuente oficial de OpenRC disponible en $(OPENRC_SRC_DIR) ($(OPENRC_COMMIT))"
	@echo "  El submódulo se mantiene fijado al commit oficial documentado en project/Documentation/OPENRC_PORT.md"

# Stage only binaries produced by a CortexOS cross build. The validator rejects
# host Linux binaries, PIE, dynamic interpreters, and non-x86_64 images.
openrc-stage: openrc-source
	$(PYTHON) project/tools/build/openrc.py --bin-dir "$(OPENRC_BIN_DIR)" --rootfs "$(OPENRC_STAGE_DIR)"

check-openrc:
	$(PYTHON) project/scripts/linux/check-openrc.py

bash-source:
	@test -f $(BASH_SRC_DIR)/configure.ac
	@grep -q 'version 5.3' $(BASH_SRC_DIR)/README
	@echo "  Fuente de GNU Bash disponible en $(BASH_SRC_DIR)"
	@echo "  Si falta, ejecuta: git submodule update --init --recursive"
	@echo "  Pendiente: libc/ABI POSIX y cargador ELF para compilarlo para CortexOS"

archinstall-source:
	@test -f project/tools/archinstall/upstream/archinstall/main.py
	@test -f project/tools/archinstall/nucleos.py
	@echo "  Archinstall upstream y adaptador CortexOS disponibles"

nucleos-archinstall:
	python3 project/tools/archinstall/nucleos.py --help

check-layout:
	$(PYTHON) project/scripts/linux/check-layout.py

check-python:
	$(PYTHON) project/scripts/linux/check-python.py

verify-crfs: $(ROOTFS)
	$(PYTHON) project/scripts/linux/verify-crfs.py $(ROOTFS)

check-live-ram:
	$(PYTHON) project/scripts/linux/check-live-ram.py qemu-serial.log

fastfetch-source:
	@test -f $(FASTFETCH_SRC_DIR)/CMakeLists.txt
	@test -f $(FASTFETCH_SRC_DIR)/LICENSE
	@echo "  Fuente oficial de Fastfetch disponible en $(FASTFETCH_SRC_DIR)"
	@echo "  Commit fijado: $(FASTFETCH_COMMIT)"
	@echo "  Pendiente: portar libc/POSIX y compilar el binario para CortexOS"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

.PHONY: all iso echo-iso run check-arch arch-list user-libc user-test-hello user-test-posix doom-source doom-platform-check doom-check tty-check aarch64-early aarch64-run armv7-early armv7-run openrc-source openrc-stage check-openrc bash-source archinstall-source nucleos-archinstall check-layout check-python verify-crfs check-live-ram fastfetch-source clean installer installer-usb

