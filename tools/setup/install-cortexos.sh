#!/usr/bin/env bash
set -euo pipefail

# CortexOS bootstrap installer. It downloads the repository snapshot and then
# executes the Linux platform script from the downloaded tree.
REPO_ARCHIVE_URL="${CORTEXOS_ARCHIVE_URL:-https://codeload.github.com/CRISTOP-bot/nucleos/legacy.tar.gz/refs/heads/main}"
INSTALL_DIR="${CORTEXOS_DIR:-${HOME}/CortexOS}"
COMMAND="${1:-check}"
shift || true

case "$COMMAND" in
  check|build|iso|qemu|clean) ;;
  *)
    echo "Uso: $0 {check|build|iso|qemu|clean} [argumentos...]" >&2
    exit 2
    ;;
esac

if [ -e "$INSTALL_DIR" ]; then
  echo "El directorio ya existe: $INSTALL_DIR" >&2
  echo "Usa CORTEXOS_DIR para elegir otra ubicación." >&2
  exit 1
fi

if ! command -v curl >/dev/null 2>&1; then
  echo "Se requiere curl para descargar CortexOS." >&2
  exit 1
fi
if ! command -v tar >/dev/null 2>&1; then
  echo "Se requiere tar para extraer CortexOS." >&2
  exit 1
fi

tmp_dir=$(mktemp -d)
cleanup() { rm -rf "$tmp_dir"; }
trap cleanup EXIT
archive="$tmp_dir/cortexos.tar.gz"

curl -fL --retry 3 --proto '=https' --tlsv1.2 "$REPO_ARCHIVE_URL" -o "$archive"
mkdir -p "$INSTALL_DIR"
tar -xzf "$archive" --strip-components=1 -C "$INSTALL_DIR"

exec bash "$INSTALL_DIR/scripts/linux/nucleos.sh" "$COMMAND" "$@"
