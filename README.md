# NucleOS

<p align="center">
  <img src="assets/branding/nucleos-logo.png" alt="NucleOS" width="280">
</p>

**Architecture:** x86_64 · **Status:** experimental · **Boot:** GRUB Multiboot v1 · **License:** GPLv3

NucleOS is an experimental operating-system project for x86_64, written
primarily in C and assembly. It is a small, freestanding environment for
studying kernel design, memory management, filesystems, processes, system
calls, userspace ABIs, and the tooling required to build a bootable system.

The main image boots through GRUB Multiboot v1 and is exercised primarily in
QEMU x86_64. NucleOS is not a general-purpose or daily-use operating system.

> The project was previously known as `cris-os-v2`.

## Current status

The table below separates verified paths from experimental code, source-only
integrations, and planned work. A component being present in the tree does not
mean that it is a usable NucleOS binary or a completed subsystem.

| Area | Status |
| --- | --- |
| x86_64 boot through GRUB Multiboot v1 | **Implemented; built and checked in CI** |
| VGA, serial, PS/2 keyboard and mouse | **Experimental implementation** |
| Physical memory manager, initial virtual memory and heap | **Experimental implementation** |
| VFS and CRFS rootfs module | **Experimental; live rootfs is copied to RAM at boot** |
| Kernel shell and TUI applications | **Experimental** |
| Static ELF support and initial libc | **Partial; suitable for small tests only** |
| `stat`, `fcntl` and `waitpid` ABI surface | **Initial ABI; compile/link smoke test** |
| Bash 5.3 | **Source integrated; NucleOS binary not available** |
| OpenRC | **Source integrated; NucleOS binary not available** |
| Fastfetch | **Source integrated; NucleOS binary not available** |
| AArch64 | **QEMU `virt` boot and PMM/MMU/GIC/EL0/SVC smoke tests; not a complete kernel** |
| ARMv7 | **Early boot smoke test; not a complete port** |
| i386 | **Port preparation only; not bootable** |
| Archinstall sources | **Upstream package vendored; NucleOS adapter is experimental** |

Major missing pieces include robust per-process address spaces, a complete
scheduler and process model, a full ELF loader, broader POSIX compatibility,
TTYs, signals, job control, dynamic linking, and a complete userspace runtime.
Bash, OpenRC, and Fastfetch are therefore not presented as working NucleOS
programs.

## Quick start

The supported development path is Linux or a Linux environment such as WSL.
For the main x86_64 build you need GCC, binutils, GNU Make, Rust, GRUB tools,
Python 3, xorriso, mtools, and QEMU.

```bash
git clone https://github.com/CRISTOP-bot/nucleos.git
cd nucleos
git submodule update --init --recursive
bash tools/setup/install-deps.sh --check
make ARCH=x86_64
make ARCH=x86_64 echo-iso
make ARCH=x86_64 run
```

The generated ISO is:

```text
dist/os.iso
```

On Windows, the repository provides CMD/PowerShell entry points. Full builds
use a native MSYS2 environment or WSL when the required GNU and GRUB tools are
not installed natively:

```bat
tools\setup\install-deps.bat --check
scripts\win\nucleos.bat iso
scripts\win\nucleos.bat virtualbox -Build
```

The VirtualBox command requires Oracle VirtualBox and `VBoxManage.exe`.

## Essential documentation

Use the README for orientation and the documents below for subsystem details.

### Architecture and layout

- [Architecture matrix](Documentation/ARCHITECTURES.md)
- [Project layout](Documentation/PROJECT_LAYOUT.md)
- [Live USB and RAM rootfs](Documentation/LIVE_RAM.md)

### Development and planning

- [Implementation plan](Documentation/IMPLEMENTATION_PLAN.md)
- [Userspace and ABI work](Documentation/USERSPACE_PORT.md)

### Ports and integrated projects

- [Bash port](Documentation/BASH_PORT.md)
- [OpenRC port](Documentation/OPENRC_PORT.md)
- [Fastfetch port](Documentation/FASTFETCH_PORT.md)

### Licensing

- [Canonical GPLv3 text](LICENSE)
- [NucleOS licensing notes](LICENSES/LICENSE.md)
- [Copyright notices](LICENSES/COPYRIGHT.md)
- [Third-party license inventory](LICENSES/THIRD_PARTY_LICENSES.md)

## Who this is for

### Trying the system

Start with the [Quick start](#quick-start), build the ISO, and run it in QEMU.
The [live RAM notes](Documentation/LIVE_RAM.md) explain when the USB can be
removed after boot.

### Studying operating systems

Read the [architecture overview](#architecture-overview), then follow the
[project layout](Documentation/PROJECT_LAYOUT.md) and the
[implementation plan](Documentation/IMPLEMENTATION_PLAN.md).

### Kernel development

The common kernel is in `kernel/`, organized by responsibility under `core/`,
`apps/`, `console/`, `graphics/`, `system/`, `services/`, and `include/`.
Memory, filesystem, IPC, initialization, networking, and block code have their
own top-level subsystems. Start with the architecture matrix before changing
boot or memory code.

### Driver development

Hardware-facing code belongs in `drivers/`, with block-device code in
`block/`. The current x86_64 path includes serial, console, PS/2, PCI, PIC, and
timer code, but these interfaces remain experimental.

### Memory, filesystems, processes, and syscalls

Use `mm/`, `fs/`, and `ipc/` as the starting points. The existing
implementation is useful for study and smoke tests, but it does not yet
provide complete process isolation or a production filesystem stack.

### Userspace, libc, and ELF

Userspace headers are in `include/`; the initial libc, `crt0`, and ABI tests
are in `usr/`. The [userspace port document](Documentation/USERSPACE_PORT.md)
describes the current boundary and its limitations.

### Architecture ports

Architecture-specific code is under `arch/`. AArch64 and ARMv7 currently have
independent early images and QEMU smoke tests; they must not be described as
complete NucleOS ports. See [ARCHITECTURES.md](Documentation/ARCHITECTURES.md).

### Build, installer, and tooling

Build and host setup code is under `tools/`; maintenance and validation helpers
are under `scripts/`. The NucleOS installer is in `tools/installer/`, while the
Archinstall-derived adapter and its upstream sources are in
`tools/archinstall/`.

### New contributors

Read the layout and implementation documents first, choose one bounded change,
run the relevant smoke tests, and document any new subsystem or architecture.
Do not turn a compile-only result into a support claim.

## Architecture overview

```text
Firmware
   |
   v
GRUB
   |
   |-- loads kernel.bin and rootfs.bin as Multiboot v1 objects into RAM
   v
arch/x86_64/boot.S
   |-- enters protected mode and long mode
   |-- installs initial page tables and stack
   v
kmain()
   |-- GDT / IDT / PIC / PIT
   |-- PMM / VMM / heap
   |-- serial, VGA, PS/2, PCI and other drivers
   |-- copy CRFS rootfs module to PMM-owned RAM
   |-- initialize VFS and experimental process/syscall/ELF paths
   v
kernel shell and experimental userspace boundary
```

`rootfs/` is the source directory containing files packaged for the live
system. `tools/build/rootfs.py` converts it into `rootfs.bin`, a compact CRFS
image. CRFS is the image format parsed by `fs/`; the VFS provides the kernel's
file and directory interface over that image. During a Multiboot live boot,
NucleOS copies the rootfs module into RAM before initializing the VFS, so normal
operation no longer depends on reading the USB.

A persistent ext2 path also exists for disk-oriented code and installation.
It is separate from the live CRFS module: ext2 is a block-backed filesystem,
whereas CRFS is the boot-time image consumed by the current VFS.

## Build, run, and test

### Build the kernel and ISO

```bash
make check-arch ARCH=x86_64
make ARCH=x86_64 clean
make ARCH=x86_64 -j"$(nproc)"
make ARCH=x86_64 iso
make ARCH=x86_64 echo-iso
```

`all` builds `build/kernel.bin`; `iso` prepares the kernel, rootfs, and
installer under `build/iso/`; `echo-iso` creates `dist/os.iso`.

### Run with QEMU

```bash
make ARCH=x86_64 run

# Or run an existing ISO directly
qemu-system-x86_64 -cdrom dist/os.iso -m 512M
```

### Serial output and debug logs

```bash
qemu-system-x86_64 \
  -cdrom dist/os.iso \
  -m 512M \
  -serial stdio \
  -no-reboot

mkdir -p build
timeout --foreground 35s \
  qemu-system-x86_64 \
  -cdrom dist/os.iso \
  -m 512M \
  -display none \
  -serial file:build/qemu-serial.log \
  -monitor none \
  -no-reboot \
  -no-shutdown \
  -d guest_errors,unimp,pcall,cpu_reset \
  -D build/qemu-debug.log || true

cat build/qemu-serial.log
cat build/qemu-debug.log
```

The kernel currently has no normal QEMU shutdown path, so a timeout can be
expected. It is only meaningful together with the serial marker and debug
logs. To verify the live rootfs marker separately:

```bash
python3 scripts/linux/check-live-ram.py build/qemu-serial.log
```

### Validation and smoke tests

```bash
make check-layout
make verify-crfs
make user-libc
make user-test-hello
make user-test-posix
make bash-source
make openrc-source
make fastfetch-source
```

The userspace targets validate small builds and ABI pieces. They do not prove
that a general POSIX application can run inside NucleOS.

### Experimental architecture images

AArch64 and ARMv7 are independent early images, not builds of the complete
x86_64 kernel:

```bash
make aarch64-early \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld

make aarch64-run \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld \
  QEMU_AARCH64=qemu-system-aarch64

make armv7-early \
  ARMV7_CC=arm-linux-gnueabihf-gcc \
  ARMV7_LD=arm-linux-gnueabihf-ld

make armv7-run \
  ARMV7_CC=arm-linux-gnueabihf-gcc \
  ARMV7_LD=arm-linux-gnueabihf-ld \
  QEMU_ARMV7=qemu-system-arm
```

`make ARCH=i386`, `make ARCH=aarch64`, and `make ARCH=armv7` intentionally
remain unavailable for the complete kernel until their ports meet the
requirements in [ARCHITECTURES.md](Documentation/ARCHITECTURES.md).

### Maintenance scripts

```bash
python3 scripts/linux/check-layout.py
python3 scripts/linux/check-python.py
python3 scripts/linux/verify-crfs.py build/iso/boot/rootfs.bin
```

The vendored Archinstall sources target their own Arch Linux environment. The
NucleOS adapter is invoked separately:

```bash
python3 tools/archinstall/nucleos.py --help
make archinstall-source
```

## Installation

Installation to a disk is experimental and destructive. Test it in a VM or on
a disposable disk before using real hardware. The installer may repartition,
format, mount, and install GRUB on the selected device.

```bash
make installer
make installer-usb

# Create a bootable USB; verify /dev/sdX first.
sudo bash tools/media/make-usb.sh /dev/sdX

# Run from a mounted ISO or USB.
sudo python3 /mnt/installer/nucleos-install

# Run from the repository.
sudo tools/installer/nucleos-install
```

BIOS and UEFI are firmware boot paths. In both cases, GRUB remains the
bootloader and uses Multiboot v1 to load NucleOS. The installation code should
not be confused with a distribution installer for daily use.

## Troubleshooting

### The kernel does not compile

Run the dependency check and verify that the selected architecture is the
intended one:

```bash
bash tools/setup/install-deps.sh --check
make check-arch ARCH=x86_64
```

On Windows, use `tools\setup\install-deps.bat --check` and build through MSYS2
or WSL if native GNU and GRUB tools are unavailable.

### The ISO is not generated

Check `grub-mkrescue`, `xorriso`, and `mtools`, then run the stages separately:

```bash
make ARCH=x86_64 iso
make ARCH=x86_64 echo-iso
```

### QEMU shows no output

Use `-serial stdio`, make sure the ISO exists, and preserve the debug logs:

```bash
qemu-system-x86_64 -cdrom dist/os.iso -m 512M -serial stdio -no-reboot
```

### The smoke test ends by timeout

The kernel is designed to continue running and currently has no standard QEMU
exit instruction. Inspect `qemu-serial.log` and `qemu-debug.log`; a timeout is
not by itself proof of a boot failure.

### Submodules are missing

Initialize them before running the source validation targets:

```bash
git submodule update --init --recursive
```

## Development

The source tree is organized by responsibility:

- `kernel/`: common kernel code, organized into `core/`, `apps/`, `console/`,
  `graphics/`, `system/`, `services/`, and `include/`.
- `arch/`: architecture-specific entry code and early ports.
- `drivers/`: `console/`, `input/`, `interrupts/`, `pci/`, and `serial/`.
- `block/`: `ata/` and `partition/` block-device support.
- `mm/`, `fs/`, `ipc/`, `init/`, and `net/`: grouped kernel subsystems.
- `lib/`: `core/` and `string/` kernel utilities; `rust/` contains Rust code
  and its C interface.
- `include/` and `usr/`: userspace headers, libc, `crt0`, and ABI tests.
- `tools/`: build, installer, Archinstall adapter, setup, LCP, and media tools.
- `scripts/`: layout, CRFS, Python, and live-RAM validation helpers.

Keep changes focused, run the smallest relevant validation target, and update
the appropriate document when adding an architecture, subsystem, or build
path. `make clean` removes only generated `build/` and `dist/` directories.

## Contributing

NucleOS is an experimental infrastructure project. A useful contribution is
small, reproducible, and explicit about its verification level.

1. Read [PROJECT_LAYOUT.md](Documentation/PROJECT_LAYOUT.md) and the relevant
   architecture or subsystem document.
2. Keep unrelated refactors out of a feature or port change.
3. Build and run the relevant smoke test before submitting a change.
4. Document new ports, hardware assumptions, or file formats.
5. Distinguish compilation, emulation, smoke testing, and actual support.
6. Preserve copyright and third-party license notices.

## Bugs, installation problems, and security

Report reproducible build failures, kernel faults, QEMU boot problems, and
installer issues through the project's
[issue tracker](https://github.com/CRISTOP-bot/nucleos/issues). Include the
architecture, host environment, command used, and relevant serial/debug logs.

There is currently no separate security-advisory or private disclosure process
documented for this project. Avoid publishing unnecessary exploit details in a
public issue; provide the smallest useful reproduction and identify the
security impact clearly so it can be triaged.

## Project resources

- [Repository](https://github.com/CRISTOP-bot/nucleos)
- [Architecture matrix](Documentation/ARCHITECTURES.md)
- [Implementation plan](Documentation/IMPLEMENTATION_PLAN.md)
- [Project layout](Documentation/PROJECT_LAYOUT.md)
- [Userspace and ABI](Documentation/USERSPACE_PORT.md)
- [Bash port](Documentation/BASH_PORT.md)
- [OpenRC port](Documentation/OPENRC_PORT.md)
- [Fastfetch port](Documentation/FASTFETCH_PORT.md)
- [Live RAM boot](Documentation/LIVE_RAM.md)
- [Issues](https://github.com/CRISTOP-bot/nucleos/issues)
- [License and third-party notices](LICENSES/THIRD_PARTY_LICENSES.md)

## License and third-party components

Original NucleOS code, documentation, and build tooling are distributed under
the GNU General Public License version 3.0. The canonical text is in
[`LICENSE`](LICENSE), with project notes in
[`LICENSES/LICENSE.md`](LICENSES/LICENSE.md) and copyright information in
[`LICENSES/COPYRIGHT.md`](LICENSES/COPYRIGHT.md).

External components retain their own licenses and are not relicensed by
NucleOS. The current inventory is maintained in
[`LICENSES/THIRD_PARTY_LICENSES.md`](LICENSES/THIRD_PARTY_LICENSES.md). It
covers the Bash, OpenRC, Fastfetch, and Archinstall sources, as well as the
submodules under `third_party/`. Preserve the relevant notices when copying,
building, or redistributing combined artifacts.
