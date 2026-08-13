#!/usr/bin/env python3
"""Compile project Python sources to catch syntax errors before CI."""
from __future__ import annotations

import py_compile
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SKIP_PARTS = {".git", "build", "dist", "third_party"}


def main() -> int:
    include_upstream = "--all" in sys.argv[1:]
    sources = []
    for path in ROOT.rglob("*.py"):
        if SKIP_PARTS & set(path.parts) or path.name == "check-python.py":
            continue
        if not include_upstream and "tools" in path.parts and "archinstall" in path.parts and "upstream" in path.parts:
            continue
        sources.append(path)
    failures = []
    for path in sorted(sources):
        try:
            py_compile.compile(str(path), doraise=True)
        except py_compile.PyCompileError as exc:
            failures.append((path, str(exc).splitlines()[-1]))
    if failures:
        for path, error in failures:
            print(f"{path}: {error}", file=sys.stderr)
        return 1
    print(f"Python syntax OK: {len(sources)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
