#!/usr/bin/env python3
"""Generate a small Make-compatible config from CortexOS Kconfig defaults."""
import re
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: kconfig.py Kconfig OUTPUT")
source, output = map(Path, sys.argv[1:])
text = source.read_text(encoding="utf-8")
configs = []
current = None
for line in text.splitlines():
    match = re.match(r"\s*config\s+([A-Z0-9_]+)\s*$", line)
    if match:
        current = match.group(1)
        configs.append((current, "n"))
        continue
    match = re.match(r"\s*default\s+(y|n)\s*$", line)
    if match and current:
        configs[-1] = (current, match.group(1))
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text("# Generated from Kconfig; edit Kconfig instead.\n" +
                  "\n".join(f"CONFIG_{name} := {value}" for name, value in configs) + "\n",
                  encoding="utf-8")
