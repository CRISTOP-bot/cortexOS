ifeq ($(shell command -v i686-elf-gcc >/dev/null 2>&1 && echo yes),yes)
	CC  = i686-elf-gcc
	AS  = i686-elf-gcc
	LD  = i686-elf-ld
else
	CC  = gcc
	AS  = gcc
	LD  = ld
endif

# Project layout
BUILD_DIR   = build
DIST_DIR    = dist
ISO_DIR     = $(BUILD_DIR)/iso
KERNEL_DIR  = kernel
ARCH_DIR    = $(KERNEL_DIR)/arch/x86_64
CORE_DIR    = $(KERNEL_DIR)/core
DRIVER_DIR  = $(KERNEL_DIR)/drivers
CONFIG_DIR  = config
GRUB_DIR    = $(CONFIG_DIR)/grub
ROOTFS_DIR  = rootfs
TOOLS_DIR   = tools

PYTHON      ?= python3
BASH_SRC_DIR = third_party/bash
OPENRC_SRC_DIR = third_party/openrc

KERNEL      = $(BUILD_DIR)/kernel.bin
ROOTFS      = $(ISO_DIR)/boot/rootfs.bin
ISO_IMAGE   = $(DIST_DIR)/os.iso
QEMU        = qemu-system-x86_64
LINKER      = $(KERNEL_DIR)/linker.ld

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

all: $(KERNEL)

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

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

.PHONY: all iso echo-iso run user-libc user-test-hello openrc-source bash-source clean installer installer-usb
