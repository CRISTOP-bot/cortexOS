#!/usr/bin/env python3
"""Check the source/staging contract for the CortexOS OpenRC port."""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXPECTED = ("openrc-init", "rc-service", "rc-status", "rc-update")


def load_stager():
    spec = importlib.util.spec_from_file_location("openrc_stager", ROOT / "tools/build/openrc.py")
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load OpenRC staging tool")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    config = ROOT / "config/openrc/cortexos-x86_64.ini"
    if not config.is_file():
        print(f"missing cross-build template: {config}", file=sys.stderr)
        return 1
    config_text = config.read_text()
    if "system = 'cortexos'" not in config_text:
        print("cross-build template does not target CortexOS", file=sys.stderr)
        return 1
    for token in ("c = 'x86_64-cortexos-gcc'", "pam = false", "selinux = 'disabled'", "pkgconfig = false"):
        if token not in config_text:
            print(f"cross-build template is missing {token}", file=sys.stderr)
            return 1
    if not (ROOT / "rootfs/etc/init.d/nucleos-console").is_file():
        print("missing CortexOS service script", file=sys.stderr)
        return 1
    if not (ROOT / "rootfs/etc/conf.d/openrc").is_file():
        print("missing CortexOS OpenRC configuration", file=sys.stderr)
        return 1
    stager = load_stager()
    if tuple(stager.PROGRAMS) != EXPECTED:
        print("staging program set changed without updating this check", file=sys.stderr)
        return 1
    print("OpenRC contract OK: source pin, CortexOS cross template, and staging validator are present")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
