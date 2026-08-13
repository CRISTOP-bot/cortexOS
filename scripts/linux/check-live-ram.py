#!/usr/bin/env python3
"""Check that a QEMU/serial log confirms the live rootfs was copied to RAM."""
from __future__ import annotations

import sys
from pathlib import Path

MARKERS = ("Rootfs copied to RAM", "Initialized VFS (RAM rootfs)")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} SERIAL_LOG", file=sys.stderr)
        return 2
    text = Path(argv[1]).read_text(encoding="utf-8", errors="replace")
    missing = [marker for marker in MARKERS if marker not in text]
    if missing:
        print("Missing live-RAM markers: " + ", ".join(missing), file=sys.stderr)
        return 1
    print("Live rootfs in RAM confirmed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
