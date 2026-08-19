#!/usr/bin/env python3
"""Check the CortexOS source-tree layout and catch stale paths."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
REQUIRED = (
    "arch",
    "boot/core",
    "boot/openrc",
    "core/kernel",
    "core/mm",
    "core/fs",
    "core/ipc",
    "core/security",
    "core/lib",
    "hw/drivers",
    "hw/block",
    "hw/net",
    "hw/sound",
    "hw/virt",
    "userspace/include",
    "userspace/usr",
    "userspace/rootfs",
    "userspace/samples",
    "project/Documentation",
    "project/scripts",
    "project/tools",
    "project/config",
    "project/third_party",
    "project/rust",
    "project/crypto",
    "project/io_uring",
    "project/assets",
    "project/certs",
    "project/LICENSES",
    "project/scripts/linux",
    "project/scripts/mac",
    "project/scripts/win",
    "project/rust/kernel",
    "project/rust/include",
    "core/kernel/core",
    "core/kernel/apps",
    "core/kernel/console",
    "core/kernel/graphics",
    "core/kernel/system",
    "core/kernel/services",
    "core/kernel/include",
    "hw/drivers/console",
    "hw/drivers/input",
    "hw/drivers/interrupts",
    "hw/drivers/pci",
    "hw/drivers/serial",
    "hw/drivers/tty",
    "hw/block/ata",
    "hw/block/partition",
    "core/fs/core",
    "core/fs/crfs",
    "core/fs/elf",
    "core/fs/ext2",
    "core/mm/physical",
    "core/mm/virtual",
    "core/mm/heap",
    "core/ipc/process",
    "core/ipc/syscall",
    "hw/net/core",
)
FORBIDDEN = (
    "Documentation", "block", "config", "crypto", "drivers", "fs", "include",
    "init", "ipc", "io_uring", "kernel", "lib", "LICENSES", "mm", "net",
    "rootfs", "rust", "samples", "scripts", "security", "sound", "third_party",
    "tools", "usr", "virt", "assets", "certs",
    "docs", "user", "LICENSE.md", "COPYRIGHT.md", "THIRD_PARTY_LICENSES.md",
)


def main() -> int:
    missing = [p for p in REQUIRED if not (ROOT / p).is_dir()]
    stale = [p for p in FORBIDDEN if (ROOT / p).exists()]
    if missing or stale:
        if missing:
            print("Missing directories: " + ", ".join(missing), file=sys.stderr)
        if stale:
            print("Obsolete paths still present: " + ", ".join(stale), file=sys.stderr)
        return 1
    required_files = (
        "Makefile",
        "README.txt",
        "project/Documentation/PROJECT_LAYOUT.md",
        "project/tools/build/rootfs.py",
        "project/tools/archinstall/nucleos.py",
        "arch/x86_64/boot.S",
    )
    missing_files = [p for p in required_files if not (ROOT / p).is_file()]
    if missing_files:
        print("Missing files: " + ", ".join(missing_files), file=sys.stderr)
        return 1
    print(f"Layout OK: {ROOT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
