#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
command_name=${1:-check}
shift || true
if command -v gmake >/dev/null 2>&1
then
  MAKE=gmake
elif command -v make >/dev/null 2>&1
then
  MAKE=make
else
  echo "No se encontró GNU Make. Instálalo antes de compilar CortexOS." >&2
  exit 1
fi
case "$command_name" in
  check)
    python3 "$ROOT/scripts/linux/check-layout.py"
    ;;
  build)
    "$MAKE" -C "$ROOT" ARCH=x86_64 all "$@"
    ;;
  iso)
    "$MAKE" -C "$ROOT" ARCH=x86_64 PYTHON=python3 echo-iso "$@"
    ;;
  qemu)
    "$MAKE" -C "$ROOT" ARCH=x86_64 PYTHON=python3 run "$@"
    ;;
  clean)
    "$MAKE" -C "$ROOT" clean "$@"
    ;;
  *)
    echo "Uso: $0 {check|build|iso|qemu|clean}" >&2
    exit 2
    ;;
esac
