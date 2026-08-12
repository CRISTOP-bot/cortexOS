#!/usr/bin/env python3
"""Build the compact CRFS image consumed by NucleOS's VFS."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

HEADER = struct.Struct("<4sIII")
ENTRY = struct.Struct("<64sII")
MAGIC = b"CRFS"
VERSION = 1


def collect(root: Path) -> list[tuple[str, bytes]]:
    files = []
    for path in sorted(root.rglob("*")):
        if path.is_file():
            name = "/" + path.relative_to(root).as_posix()
            if len(name.encode()) >= 64:
                raise ValueError(f"rootfs path is too long: {name}")
            files.append((name, path.read_bytes()))
    return files


def build(source: Path, output: Path) -> None:
    files = collect(source)
    header_size = HEADER.size + ENTRY.size * len(files)
    entries = []
    payload = bytearray()
    offset = header_size
    for name, data in files:
        field = name.encode() + b"\0"
        entries.append(ENTRY.pack(field.ljust(64, b"\0"), offset, len(data)))
        payload.extend(data)
        offset += len(data)
    image = bytearray(HEADER.pack(MAGIC, VERSION, len(files), 0))
    image.extend(b"".join(entries))
    image.extend(payload)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    print(f"  Rootfs CRFS: {output} ({len(files)} files, {len(image)} bytes)")


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(f"usage: {argv[0]} ROOTFS_DIR OUTPUT", file=sys.stderr)
        return 2
    build(Path(argv[1]), Path(argv[2]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
