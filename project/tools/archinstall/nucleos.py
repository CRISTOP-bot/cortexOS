#!/usr/bin/env python3
"""CortexOS installer adapter.

The upstream Archinstall sources are vendored under ``upstream/`` for reuse of
its UI and configuration ideas, but Arch Linux operations (pacman, systemd,
arch-chroot and Arch package databases) are intentionally not called here.
CortexOS uses its own CRFS, GRUB and ext2 installer backend instead.
"""
from __future__ import annotations

import argparse
import getpass
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOOLS = ROOT / "project/tools"
sys.path.insert(0, str(TOOLS))

from installer.config import CortexOSConfig, PartitionConfig, UserConfig  # noqa: E402
from installer.install import install  # noqa: E402


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="nucleos-archinstall",
        description="Instalador declarativo de CortexOS basado en CRFS y GRUB.",
    )
    p.add_argument("--device", help="Disco completo para particionado automático")
    p.add_argument("--root", dest="root_device", help="Partición root existente")
    p.add_argument("--boot", dest="boot_device", help="Partición boot/EFI existente")
    p.add_argument("--mountpoint", default="/mnt/cortexos")
    p.add_argument("--fstype", choices=("ext2",), default="ext2")
    p.add_argument("--uefi", action="store_true")
    p.add_argument("--existing", action="store_true", help="Usar particiones existentes")
    p.add_argument("--hostname", default="cortexos")
    p.add_argument("--kernel", default=str(ROOT / "build/kernel.bin"))
    p.add_argument("--rootfs", default=str(ROOT / "userspace/rootfs"))
    p.add_argument("--root-password-file")
    p.add_argument("--user", action="append", metavar="NAME:PASSWORD")
    p.add_argument("--plan", action="store_true", help="Mostrar el plan sin tocar discos")
    p.add_argument("--install", action="store_true", help="Ejecutar la instalación destructiva")
    return p


def read_password(path: str | None, label: str) -> str:
    if path:
        value = Path(path).read_text(encoding="utf-8").splitlines()[0]
    else:
        value = getpass.getpass(label)
    if not value:
        raise ValueError("La contraseña no puede estar vacía")
    return value


def make_config(args: argparse.Namespace) -> CortexOSConfig:
    automatic = not args.existing
    device = args.device or args.root_device or ""
    if automatic and not args.device:
        raise ValueError("--device es obligatorio para particionado automático")
    if not automatic and (not args.root_device or not args.boot_device):
        raise ValueError("--root y --boot son obligatorios con --existing")

    users = []
    for raw in args.user or []:
        if ":" not in raw:
            raise ValueError("Cada --user debe tener el formato NAME:PASSWORD")
        name, password = raw.split(":", 1)
        users.append(UserConfig(username=name, password=password))

    password = read_password(args.root_password_file, "Contraseña root: ")
    partition = PartitionConfig(
        device=device,
        disk_device=args.device or "",
        boot_device=args.boot_device or "",
        fstype=args.fstype,
        mountpoint=args.mountpoint,
        uefi=args.uefi,
        auto_partition=automatic,
    )
    return CortexOSConfig(
        kernel_source=args.kernel,
        rootfs_source=args.rootfs,
        hostname=args.hostname,
        root_password=password,
        users=users,
        partition=partition,
    )


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    if not args.plan and not args.install:
        print("Usa --plan para revisar o --install para ejecutar la instalación.")
        return 2
    try:
        config = make_config(args)
        print("Plan de instalación CortexOS:")
        print("\n".join(f"  {line}" for line in config.summary_lines()))
        if args.plan:
            print("Modo plan: no se modificaron discos.")
            return 0
        if os.geteuid() != 0:
            print("La instalación requiere root.", file=sys.stderr)
            return 1
        if input("Escribe INSTALAR para continuar: ") != "INSTALAR":
            print("Instalación cancelada.")
            return 0
        return 0 if install(config) else 1
    except (OSError, ValueError, IndexError) as exc:
        print(f"Error de configuración: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
