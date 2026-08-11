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
KERNEL_DIR  = kernel
ARCH_DIR    = $(KERNEL_DIR)/arch/$(ARCH)
CORE_DIR    = $(KERNEL_DIR)/core
DRIVER_DIR  = $(KERNEL_DIR)/drivers
CONFIG_DIR  = config
GRUB_DIR    = $(CONFIG_DIR)/grub
ROOTFS_DIR  = rootfs
TOOLS_DIR   = tools

PYTHON      ?= python3
BASH_SRC_DIR    = third_party/bash
OPENRC_SRC_DIR  = third_party/openrc
FASTFETCH_SRC_DIR = third_party/fastfetch
FASTFETCH_COMMIT  = a0452b8323aaa9d3b5b6ded435ed6660cee2bbb9

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
AARCH64_OBJECTS = $(AARCH64_BUILD)/boot.o $(AARCH64_BUILD)/early.o \
	$(AARCH64_BUILD)/fdt.o $(AARCH64_BUILD)/mmu.o $(AARCH64_BUILD)/gic.o

CFLAGS  = -ffreestanding -O2 -Wall -Wextra -m64 -nostdlib -std=c99 -I $(CORE_DIR) -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -fno-strict-aliasing -mno-red-zone -mcmodel=kernel -fno-pic -fno-pie
ASFLAGS = -m64 -ffreestanding
LDFLAGS = -m elf_x86_64 -nostdlib

RUSTC       = rustc
RUST_TARGET = x86_64-unknown-linux-gnu
RUSTFLAGS   = -C no-redzone=yes -C code-model=kernel -C relocation-model=static
RUSTFLAGS  += -C panic=abort -C debuginfo=0 -C opt-level=2
RUST_SRC    = $(CORE_DIR)/rust/rust_kernel.rs
RUST_OBJ    = $(BUILD_DIR)/rust_kernel.o

# Adding a C file to kernel/core or kernel/drivers automatically includes it.
CORE_SRCS   = $(wildcard $(CORE_DIR)/*.c)
DRIVER_SRCS = $(wildcard $(DRIVER_DIR)/*.c)
CORE_OBJS   = $(patsubst $(CORE_DIR)/%.c,$(BUILD_DIR)/core_%.o,$(CORE_SRCS))
DRIVER_OBJS = $(patsubst $(DRIVER_DIR)/%.c,$(BUILD_DIR)/drv_%.o,$(DRIVER_SRCS))
ARCH_OBJS   = $(BUILD_DIR)/arch_asm_utils.o \
              $(BUILD_DIR)/arch_math_asm.o \
              $(BUILD_DIR)/arch_ctx_switch.o
OBJS        = $(CORE_OBJS) $(DRIVER_OBJS) $(ARCH_OBJS) \
              $(BUILD_DIR)/boot_entry.o $(RUST_OBJ)

INSTALLER_FILES = tools/installer/__init__.py \
                  tools/installer/ui.py \
                  tools/installer/disk.py \
                  tools/installer/config.py \
                  tools/installer/install.py \
                  tools/installer/nucleos-install
LCP_FILES      = tools/lcp/lcp.py tools/lcp/main_repo.json

all: check-arch $(KERNEL)

check-arch:
ifeq ($(ARCH_SUPPORTED),yes)
	@echo "  Arquitectura seleccionada: $(ARCH)"
else
	$(error ARCH=$(ARCH) todavía no tiene un port arrancable; consulta docs/ARCHITECTURES.md)
endif

arch-list:
	@echo "Arquitecturas declaradas: x86_64 i386 aarch64 armv7"
	@echo "Kernel completo arrancable: x86_64"
	@echo "Etapa de boot independiente: aarch64"
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

$(BUILD_DIR)/core_%.o: $(CORE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drv_%.o: $(DRIVER_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) $(LINKER) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -T $(LINKER) -o $@ $(OBJS)

$(ISO_DIR)/installer: $(INSTALLER_FILES) $(LCP_FILES) | $(ISO_DIR)/boot/grub
	mkdir -p $@
	cp -r tools/installer/. $@/
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
	@echo "    sudo tools/installer/nucleos-install"
	@echo ""
	@echo "  Debes compilar primero: make echo-iso"
	@echo ""

installer-usb:
	@echo ""
	@echo "  Crear USB booteable con instalador:"
	@echo "    sudo bash tools/media/make-usb.sh /dev/sdX"
	@echo ""
	@echo "  ADVERTENCIA: Esto BORRA todos los datos del dispositivo"
	@echo ""

user-libc:
	$(MAKE) -C user CC="$(CC)"

user-test-hello:
	$(MAKE) -C user CC="$(CC)" test-hello

user-test-posix:
	$(MAKE) -C user CC="$(CC)" test-posix

# First independently buildable AArch64 stage. This does not yet build the
# x86_64-oriented kernel/core; it validates the ARM64 boot and UART path.
aarch64-early: $(AARCH64_EARLY)
	@echo "  AArch64 early image: $(AARCH64_EARLY)"

$(AARCH64_BUILD):
	mkdir -p $@

$(AARCH64_BUILD)/boot.o: $(KERNEL_DIR)/arch/aarch64/boot.S | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -march=armv8-a $< -o $@

$(AARCH64_BUILD)/early.o: $(KERNEL_DIR)/arch/aarch64/early.c $(KERNEL_DIR)/arch/aarch64/fdt.h $(KERNEL_DIR)/arch/aarch64/mmu.h $(KERNEL_DIR)/arch/aarch64/gic.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -I$(KERNEL_DIR)/arch/aarch64 $< -o $@

$(AARCH64_BUILD)/fdt.o: $(KERNEL_DIR)/arch/aarch64/fdt.c $(KERNEL_DIR)/arch/aarch64/fdt.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -I$(KERNEL_DIR)/arch/aarch64 $< -o $@

$(AARCH64_BUILD)/mmu.o: $(KERNEL_DIR)/arch/aarch64/mmu.c $(KERNEL_DIR)/arch/aarch64/mmu.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -I$(KERNEL_DIR)/arch/aarch64 $< -o $@

$(AARCH64_BUILD)/gic.o: $(KERNEL_DIR)/arch/aarch64/gic.c $(KERNEL_DIR)/arch/aarch64/gic.h | $(AARCH64_BUILD)
	$(AARCH64_CC) -c -ffreestanding -nostdlib -std=c99 -Wall -Wextra -march=armv8-a -I$(KERNEL_DIR)/arch/aarch64 $< -o $@

$(AARCH64_EARLY): $(AARCH64_OBJECTS) $(KERNEL_DIR)/arch/aarch64/linker.ld
	$(AARCH64_LD) -nostdlib -T $(KERNEL_DIR)/arch/aarch64/linker.ld -o $@ $(AARCH64_OBJECTS)

# QEMU virt exposes the PL011 UART at 0x09000000.
aarch64-run: aarch64-early
	$(QEMU_AARCH64) -machine virt -cpu cortex-a57 -m 128M -nographic -monitor none -serial stdio -no-reboot -kernel $(AARCH64_EARLY)

openrc-source:
	@test -f $(OPENRC_SRC_DIR)/meson.build
	@test -d $(OPENRC_SRC_DIR)/src
	@echo "  Fuente oficial de OpenRC disponible en $(OPENRC_SRC_DIR)"
	@echo "  El submódulo se mantiene fijado al commit oficial documentado en docs/OPENRC_PORT.md"

bash-source:
	@test -f $(BASH_SRC_DIR)/configure.ac
	@grep -q 'version 5.3' $(BASH_SRC_DIR)/README
	@echo "  Fuente de GNU Bash disponible en $(BASH_SRC_DIR)"
	@echo "  Si falta, ejecuta: git submodule update --init --recursive"
	@echo "  Pendiente: libc/ABI POSIX y cargador ELF para compilarlo para NucleOS"

fastfetch-source:
	@test -f $(FASTFETCH_SRC_DIR)/CMakeLists.txt
	@test -f $(FASTFETCH_SRC_DIR)/LICENSE
	@echo "  Fuente oficial de Fastfetch disponible en $(FASTFETCH_SRC_DIR)"
	@echo "  Commit fijado: $(FASTFETCH_COMMIT)"
	@echo "  Pendiente: portar libc/POSIX y compilar el binario para NucleOS"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

.PHONY: all iso echo-iso run check-arch arch-list user-libc user-test-hello user-test-posix aarch64-early aarch64-run openrc-source bash-source fastfetch-source clean installer installer-usb

