#!/usr/bin/env python3
"""Validate a CortexOS CRFS image without mounting or modifying anything."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

HEADER = struct.Struct("<4sIII")
ENTRY = struct.Struct("<64sII")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} IMAGE", file=sys.stderr)
        return 2
    image_path = Path(argv[1])
    data = image_path.read_bytes()
    if len(data) < HEADER.size:
        raise SystemExit("CRFS image is truncated")
    magic, version, count, _ = HEADER.unpack_from(data)
    if magic != b"CRFS" or version != 1:
        raise SystemExit("invalid CRFS header")
    table_end = HEADER.size + count * ENTRY.size
    if table_end > len(data):
        raise SystemExit("CRFS entry table is truncated")
    names: set[str] = set()
    for index in range(count):
        raw_name, offset, size = ENTRY.unpack_from(data, HEADER.size + index * ENTRY.size)
        name = raw_name.split(b"\0", 1)[0].decode("utf-8")
        if not name or name in names:
            raise SystemExit(f"invalid or duplicate entry {index}: {name!r}")
        if offset < table_end or offset > len(data) or size > len(data) - offset:
            raise SystemExit(f"entry outside image: {name}")
        names.add(name)
    print(f"CRFS OK: {image_path} ({count} files, {len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
