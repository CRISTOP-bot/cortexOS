#!/usr/bin/env python3
"""Check the source tree layout and catch stale paths after refactors."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = (
    "Documentation",
    "arch",
    "block",
    "drivers",
    "fs",
    "include",
    "init",
    "ipc",
    "kernel",
    "lib",
    "mm",
    "net",
    "rootfs",
    "rust",
    "scripts",
    "tools",
    "usr",
)
FORBIDDEN = (
    "docs",
    "user",
    "kernel/arch",
    "kernel/drivers",
    "LICENSE.md",
    "COPYRIGHT.md",
    "THIRD_PARTY_LICENSES.md",
)


def main() -> int:
    required = REQUIRED + (
        "kernel/core",
        "kernel/apps",
        "kernel/console",
        "kernel/graphics",
        "kernel/system",
        "kernel/services",
        "kernel/include",
        "drivers/console",
        "drivers/input",
        "drivers/interrupts",
        "drivers/pci",
        "drivers/serial",
        "block/ata",
        "block/partition",
        "fs/core",
        "fs/crfs",
        "fs/elf",
        "fs/ext2",
        "mm/physical",
        "mm/virtual",
        "mm/heap",
        "ipc/process",
        "ipc/syscall",
        "init/core",
        "init/openrc",
        "net/core",
        "lib/core",
        "lib/string",
        "rust/kernel",
        "rust/include",
    )
    missing = [p for p in required if not (ROOT / p).is_dir()]
    stale = [p for p in FORBIDDEN if (ROOT / p).exists()]
    if list((ROOT / "kernel").glob("*.c")) or list((ROOT / "kernel").glob("*.h")):
        stale.append("kernel files outside functional subdirectories")
    if missing or stale:
        if missing:
            print("Missing directories: " + ", ".join(missing), file=sys.stderr)
        if stale:
            print("Obsolete paths still present: " + ", ".join(stale), file=sys.stderr)
        return 1
    required_files = (
        "Makefile",
        "Documentation/PROJECT_LAYOUT.md",
        "tools/build/rootfs.py",
        "tools/archinstall/nucleos.py",
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
