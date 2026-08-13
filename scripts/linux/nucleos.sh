#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
command_name=${1:-check}
shift || true
case "$command_name" in
  check)
    python3 "$ROOT/scripts/linux/check-layout.py"
    ;;
  build)
    make -C "$ROOT" ARCH=x86_64 all "$@"
    ;;
  iso)
    make -C "$ROOT" ARCH=x86_64 PYTHON=python3 echo-iso "$@"
    ;;
  qemu)
    make -C "$ROOT" ARCH=x86_64 PYTHON=python3 run "$@"
    ;;
  clean)
    make -C "$ROOT" clean "$@"
    ;;
  *)
    echo "Uso: $0 {check|build|iso|qemu|clean}" >&2
    exit 2
    ;;
esac
