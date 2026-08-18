CortexOS
=======

CortexOS is an experimental operating-system project written primarily in C and
assembly. It is designed for studying kernel design, boot processes, memory
management, filesystems, processes, system calls, userspace ABIs, drivers, and
the tooling required to build a bootable system.

The primary bootable path is x86_64 through GRUB Multiboot v1 and QEMU. CortexOS
is not a general-purpose or daily-use operating system.

Project repository: https://github.com/CRISTOP-bot/nucleos
Issue tracker: https://github.com/CRISTOP-bot/nucleos/issues
Previous project name: cris-os-v2

Project Status
==============

The following status describes the implementation honestly. A component being
present in the source tree does not mean that it is complete or available as a
working CortexOS binary.

Implemented or verified at the current level:

* x86_64 GRUB Multiboot v1 entry and boot path.
* VGA console and serial output.
* PS/2 keyboard and mouse paths.
* Experimental physical memory manager, initial virtual memory, and heap.
* Experimental VFS and CRFS root filesystem module.
* Experimental kernel shell and TUI applications.
* Static ELF support and an initial libc suitable only for small tests.
* Initial stat, fcntl, and waitpid ABI surfaces with compile/link smoke tests.
* Live rootfs copy from the Multiboot module into PMM-owned RAM.
* Windows CMD and PowerShell wrappers.
* Experimental installer, VirtualBox runners, and Archinstall-derived tooling.
* Layout, CRFS, Python, and live-RAM validation scripts.
* Independent AArch64 and ARMv7 early-boot images for QEMU testing.

Not complete:

* Robust per-process address spaces and complete process isolation.
* A complete scheduler, context switching, and process model.
* A full ELF loader and broad POSIX compatibility.
* TTYs, signals, job control, dynamic linking, and a complete userspace runtime.
* Bash, OpenRC, and Fastfetch as working CortexOS binaries.
* A playable Doom binary. The GPLv2 engine source is pinned as a submodule, but
  the CortexOS platform boundary is not yet wired to userspace; no IWAD/game
  data is included and a compatible IWAD must be supplied separately.
* An integrated ARM64 CortexOS kernel and rootfs image.
* Complete ARMv7 and i386 ports.

AArch64 currently provides an independent QEMU virt early image with FDT, MMU,
PMM, GICv2, Generic Timer, IRQ, EL0, and SVC smoke coverage. ARMv7 provides an
early boot image with vectors, VBAR, UART PL011, and initial FDT validation.
Neither is a complete CortexOS port. i386 remains port preparation only.

Quick Start
===========

The main development path is Linux or a Linux environment such as WSL. The
x86_64 build normally requires GCC, binutils, GNU Make, Rust, GRUB tools,
Python 3, xorriso, mtools, and QEMU.

Clone and initialize the repository:

    git clone https://github.com/CRISTOP-bot/nucleos.git
    cd nucleos
    git submodule update --init --recursive
    bash tools/setup/install-deps.sh --check

Build and run the main x86_64 image:

    make check-arch ARCH=x86_64
    make ARCH=x86_64 clean
    make ARCH=x86_64 -j"$(nproc)"
    make ARCH=x86_64 iso
    make ARCH=x86_64 run

The generated ISO is:

    dist/os.iso

The platform-specific script entry points are:

Linux:

    scripts/linux/nucleos.sh check
    scripts/linux/nucleos.sh build
    scripts/linux/nucleos.sh iso
    scripts/linux/nucleos.sh qemu

macOS:

    scripts/mac/nucleos.sh check
    scripts/mac/nucleos.sh build
    scripts/mac/nucleos.sh iso
    scripts/mac/nucleos.sh qemu

The macOS script uses gmake when available. Building an ISO still requires the
GNU, GRUB, xorriso, and mtools tools required by the project.

Windows:

    tools\setup\install-deps.bat --check
    scripts\win\nucleos.bat check
    scripts\win\nucleos.bat iso
    scripts\win\nucleos.bat virtualbox -Build

Windows builds use native MSYS2 tools or WSL when GNU and GRUB tools are not
available natively. VirtualBox commands require Oracle VirtualBox and
VBoxManage.exe.

Essential Documentation
=======================

Use this file for project orientation. Detailed subsystem information is kept
in Documentation/.

Architecture and layout:

* Documentation/ARCHITECTURES.md - supported and experimental architecture paths.
* Documentation/PROJECT_LAYOUT.md - source-tree responsibilities and locations.
* Documentation/LIVE_RAM.md - live rootfs loading and RAM behavior.

Development and planning:

* Documentation/IMPLEMENTATION_PLAN.md - current implementation roadmap.
* Documentation/USERSPACE_PORT.md - userspace, libc, ELF, and ABI limitations.
* Documentation/DOOM_PORT.md - Doom milestone status, controls, licensing, and blockers.

Integrated projects:

* Documentation/BASH_PORT.md - Bash source integration status.
* Documentation/OPENRC_PORT.md - OpenRC source integration status.
* Documentation/FASTFETCH_PORT.md - Fastfetch source integration status.

Licensing:

* LICENSE - GPLv3 license for CortexOS code and original project files.
* LICENSES/LICENSE.md - project licensing notes.
* LICENSES/COPYRIGHT.md - copyright notices.
* LICENSES/THIRD_PARTY_LICENSES.md - external component license inventory.

Who Are You?
============

Find the section below that best matches your work with CortexOS:

* New OS Developer - learning how kernels, bootloaders, memory, and userspace
  fit together.
* Academic Researcher - studying operating-system architecture and experiments.
* Kernel Developer - working on core code, processes, memory, filesystems, or
  system calls.
* Driver Developer - adding hardware support and architecture-specific paths.
* Userspace Developer - working on libc, ELF programs, ABI tests, and rootfs
  applications.
* Tooling Developer - improving the Makefile, validation scripts, installers,
  CI, or platform wrappers.
* Contributor - fixing a bounded issue, improving documentation, or adding a
  tested subsystem.
* AI Coding Assistant - following the project documentation, preserving the
  stated implementation status, and reporting exactly what was verified.

For Specific Users
==================

New OS Developer
----------------

Start with the Quick Start section and run the x86_64 image in QEMU. Then read:

* Documentation/ARCHITECTURES.md
* Documentation/PROJECT_LAYOUT.md
* Documentation/IMPLEMENTATION_PLAN.md
* Documentation/LIVE_RAM.md

Use QEMU or another disposable virtual machine. The installer is experimental
and destructive; never test it first on a disk containing important data.

Academic Researcher
-------------------

The most relevant experimental areas are:

* arch/ - architecture-specific boot and port code.
* mm/ - physical memory, virtual memory, and heap code.
* ipc/ - process and system-call code.
* fs/ - VFS, CRFS, ELF, and ext2 code.
* drivers/ and block/ - hardware and block-device paths.
* Documentation/IMPLEMENTATION_PLAN.md - known gaps and planned work.

Results from an early boot image, compilation, or smoke test must not be
presented as evidence of a complete operating-system feature.

Kernel Developer
----------------

The common kernel is organized by responsibility under kernel/:

* kernel/core/ - common entry and boot logic.
* kernel/apps/ - applications and application registry code.
* kernel/console/ - console interface and shell.
* kernel/graphics/ - experimental GUI code.
* kernel/system/ - GDT, IDT, TSS, LCP, and persistence-related code.
* kernel/services/ - installer and service-related kernel components.
* kernel/include/ - kernel-local headers.

Other kernel subsystems remain separate at the repository root. Read
Documentation/PROJECT_LAYOUT.md before moving code or changing build paths.

Driver Developer
----------------

Hardware-facing code is organized under drivers/:

* drivers/console/ - console output.
* drivers/input/ - keyboard and mouse paths.
* drivers/interrupts/ - PIC and timer paths.
* drivers/pci/ - PCI support.
* drivers/serial/ - serial output.
* block/ata/ and block/partition/ - block-device support.

These interfaces are experimental and primarily exercised on x86_64 in QEMU.

Memory, Filesystems, Processes, and Syscalls
---------------------------------------------

Use these directories as starting points:

* mm/physical/ - physical memory management.
* mm/virtual/ - initial virtual-memory management.
* mm/heap/ - heap and memory helpers.
* fs/core/ - VFS code.
* fs/crfs/ - CRFS image support.
* fs/elf/ - ELF support.
* fs/ext2/ - ext2-related code.
* ipc/process/ - process code.
* ipc/syscall/ - system-call code.

The current implementations are useful for study and smoke tests, but they do
not yet provide complete process isolation or a production filesystem stack.

Userspace, libc, and ELF
------------------------

Public userspace ABI headers are in include/. The initial libc, crt0, and ABI
tests are in usr/. Static ELF support is partial and is intended for small
experiments. Dynamic linking and broad POSIX compatibility are not implemented.

Architecture Ports
------------------

Architecture-specific code is under arch/. The main bootable path is:

* x86_64 - primary CortexOS kernel image through GRUB Multiboot v1.
* aarch64 - independent QEMU virt early image, not a complete kernel.
* armv7 - independent early boot image, not a complete port.
* i386 - port preparation only, not bootable.

See Documentation/ARCHITECTURES.md before describing an architecture as
supported.

Build, Installer, and Tooling Developer
---------------------------------------

* tools/ - build, installer, media, Archinstall adapter, and setup tooling.
* scripts/linux/ - Linux validation and build helpers.
* scripts/mac/ - macOS build entry point.
* scripts/win/ - Windows CMD, PowerShell, and VirtualBox entry points.
* usr/ - userspace runtime and tests.
* third_party/ - external source integrations and submodules.

The installer may partition, format, mount, and install GRUB. Test it only in a
virtual machine or on a disposable disk.

New Contributor
---------------

Choose one bounded change, read the relevant documentation, update the layout
or build files when necessary, and run the closest available validation:

    make check-layout
    python3 scripts/linux/check-python.py
    make verify-crfs
    make user-test-posix

For boot changes, also run the relevant QEMU smoke test and inspect serial
output. Do not turn a compile-only result into a support claim.

Architecture Overview
=====================

    Firmware
       |
       v
    GRUB Multiboot v1
       |
       |-- loads kernel.bin and rootfs.bin into RAM
       v
    arch/x86_64/boot.S
       |-- enters the x86_64 boot path
       |-- installs initial page tables and stack
       v
    kmain()
       |-- initializes GDT, IDT, PIC, and timer paths
       |-- initializes PMM, VMM, and heap
       |-- initializes serial, VGA, PS/2, PCI, and other drivers
       |-- copies the CRFS rootfs module to PMM-owned RAM
       |-- initializes the VFS and experimental process/syscall/ELF paths
       v
    kernel shell and experimental userspace boundary

rootfs/ contains the files packaged into the live system. tools/build/rootfs.py
converts them into rootfs.bin, a compact CRFS image. During a live Multiboot
boot, CortexOS copies the rootfs module into RAM before initializing the VFS.
Changes made to the live rootfs are temporary and are lost at shutdown unless a
separate persistence mechanism is used.

An experimental ext2 path also exists for disk-oriented code and installation.
It is separate from the live CRFS module: ext2 is block-backed, while CRFS is
the boot-time image consumed by the current VFS.

Build, Run, and Test
====================

Build the kernel and ISO:

    make check-arch ARCH=x86_64
    make ARCH=x86_64 clean
    make ARCH=x86_64 -j"$(nproc)"
    make ARCH=x86_64 iso
    make ARCH=x86_64 echo-iso

Run with QEMU:

    make ARCH=x86_64 run
    qemu-system-x86_64 -cdrom dist/os.iso -m 512M

Userspace tests:

    make user-libc
    make user-test-hello
    make user-test-posix

Validation:

    make check-layout
    make verify-crfs
    python3 scripts/linux/check-python.py
    python3 scripts/linux/check-live-ram.py build/qemu-serial.log

Early architecture images:

    make aarch64-early AARCH64_CC=aarch64-linux-gnu-gcc AARCH64_LD=aarch64-linux-gnu-ld
    make aarch64-run AARCH64_CC=aarch64-linux-gnu-gcc AARCH64_LD=aarch64-linux-gnu-ld
    make armv7-early ARMV7_CC=arm-linux-gnueabihf-gcc ARMV7_LD=arm-linux-gnueabihf-ld
    make armv7-run ARMV7_CC=arm-linux-gnueabihf-gcc ARMV7_LD=arm-linux-gnueabihf-ld

Remote Bootstrap Installers
===========================

CortexOS provides bootstrap installers that download a repository snapshot and
then execute the platform script from the downloaded tree. They do not
partition or format disks. The default installation directories are
`$HOME/CortexOS` on Linux/macOS and `$HOME\CortexOS` on Windows.

Linux or macOS:

    curl -fsSL https://raw.githubusercontent.com/CRISTOP-bot/nucleos/5839ba06f44023eeaa37dd975316d78371c300cf/tools/setup/install-cortexos.sh | bash -s -- check

Replace `check` with `build`, `iso`, `qemu`, or `clean` to execute another
platform command. To choose another directory, set `CORTEXOS_DIR`:

    CORTEXOS_DIR="$HOME/CortexOS-dev" bash -c "$(curl -fsSL https://raw.githubusercontent.com/CRISTOP-bot/nucleos/5839ba06f44023eeaa37dd975316d78371c300cf/tools/setup/install-cortexos.sh)" -- check

Windows PowerShell:

    $installer = Join-Path $env:TEMP 'install-cortexos.ps1'
    curl.exe -fsSL https://raw.githubusercontent.com/CRISTOP-bot/nucleos/5839ba06f44023eeaa37dd975316d78371c300cf/tools/setup/install-cortexos.ps1 -o $installer
    powershell.exe -ExecutionPolicy Bypass -File $installer -Command check

The PowerShell installer accepts `-Command build`, `-Command iso`,
`-Command qemu`, or `-Command clean`, and supports `-InstallDir` for a custom
installation directory. Review remote scripts before piping them to a shell.
The bootstrap archive can be overridden with `CORTEXOS_ARCHIVE_URL` for a
reviewed branch or release.

Installation
============

The installer is experimental and destructive. It may partition, format,
mount, and install GRUB. Test it first in a virtual machine or on a disposable
disk.

    make installer
    make installer-usb
    sudo bash tools/media/make-usb.sh /dev/sdX
    sudo python3 /mnt/installer/nucleos-install
    sudo tools/installer/nucleos-install

BIOS and UEFI are firmware paths. GRUB remains responsible for loading CortexOS
through Multiboot v1.

Communication and Support
=========================

* Issues and bug reports: https://github.com/CRISTOP-bot/nucleos/issues
* Source code and changes: https://github.com/CRISTOP-bot/nucleos
* Project layout: Documentation/PROJECT_LAYOUT.md
* Architecture status: Documentation/ARCHITECTURES.md

When reporting a problem, include the host platform, architecture, command
used, relevant build output, and QEMU serial output when applicable. Explain
whether the result was a compilation check, a smoke test, or a real runtime
observation.

License
=======

CortexOS code, documentation, build scripts, and original files are licensed
under GPLv3. Third-party components retain their original licenses. See:

* LICENSE
* LICENSES/LICENSE.md
* LICENSES/COPYRIGHT.md
* LICENSES/THIRD_PARTY_LICENSES.md
