#!/usr/bin/env python3
"""Validate and stage already cross-built OpenRC user programs.

This tool intentionally does not compile OpenRC. A host OpenRC executable
would use the host Linux ABI and would be unsafe to put in a CortexOS image.
Once a CortexOS libc/toolchain exists, its output can be staged with:

    make openrc-stage OPENRC_BIN_DIR=/path/to/cortexos/openrc/bin

The validator only accepts static x86_64 ELF executables because that is the
only user ABI the current CortexOS loader can load.
"""
from __future__ import annotations

import argparse
import shutil
import struct
import sys
from pathlib import Path

PROGRAMS = {
    "openrc-init": Path("sbin/openrc-init"),
    "rc-service": Path("sbin/rc-service"),
    "rc-status": Path("sbin/rc-status"),
    "rc-update": Path("sbin/rc-update"),
}
ELF_HEADER = struct.Struct("<16sHHIQQQIHHHHHH")
PROGRAM_HEADER = struct.Struct("<IIQQQQQQ")
ELF_MAGIC = b"\x7fELF"
PT_LOAD = 1
PT_INTERP = 3
ET_EXEC = 2
EM_X86_64 = 62


def validate_static_x86_64(path: Path) -> list[str]:
    data = path.read_bytes()
    if len(data) < ELF_HEADER.size:
        return ["file is shorter than an ELF64 header"]
    (ident, kind, machine, version, entry, phoff, _shoff, _flags,
     _ehsize, phentsize, phnum, *_rest) = ELF_HEADER.unpack_from(data)
    errors: list[str] = []
    load_segments = 0
    if ident[:4] != ELF_MAGIC or ident[4] != 2 or ident[5] != 1:
        errors.append("not a little-endian ELF64 image")
    if kind != ET_EXEC:
        errors.append(f"ELF type {kind} is not ET_EXEC (the loader does not support PIE/ET_DYN)")
    if machine != EM_X86_64:
        errors.append(f"ELF machine {machine} is not x86_64")
    if version != 1:
        errors.append("unsupported ELF version")
    if phentsize < PROGRAM_HEADER.size or phoff + phentsize * phnum > len(data):
        errors.append("invalid program-header table")
    else:
        for index in range(phnum):
            off = phoff + index * phentsize
            p_type, _flags, p_offset, _vaddr, _paddr, p_filesz, _memsz, _align = PROGRAM_HEADER.unpack_from(data, off)
            if p_type == PT_LOAD:
                load_segments += 1
                if p_filesz > _memsz:
                    errors.append("load segment has file size larger than memory size")
                if entry == 0:
                    errors.append("loadable image has no valid entry point")
            if p_type == PT_INTERP:
                interpreter = data[p_offset:p_offset + p_filesz].split(b"\0", 1)[0]
                errors.append(f"dynamic interpreter {interpreter.decode(errors='replace')!r} is unsupported")
            if p_offset + p_filesz > len(data):
                errors.append("program segment extends beyond the file")
        if load_segments == 0:
            errors.append("ELF has no PT_LOAD segment")
    return errors


def stage(bin_dir: Path, rootfs: Path) -> int:
    missing = [name for name in PROGRAMS if not (bin_dir / name).is_file()]
    if missing:
        print("OpenRC staging requires cross-built files: " + ", ".join(missing), file=sys.stderr)
        print("Build them with the CortexOS toolchain first; host OpenRC binaries are rejected.", file=sys.stderr)
        return 2
    for name, destination in PROGRAMS.items():
        source = bin_dir / name
        errors = validate_static_x86_64(source)
        if errors:
            print(f"{source}: " + "; ".join(errors), file=sys.stderr)
            return 2
        target = rootfs / destination
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        target.chmod(0o755)
        print(f"  staged CortexOS OpenRC: /{destination}")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin-dir", type=Path, required=True)
    parser.add_argument("--rootfs", type=Path, required=True)
    args = parser.parse_args(argv)
    return stage(args.bin_dir, args.rootfs)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
