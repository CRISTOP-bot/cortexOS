from __future__ import annotations

import hashlib
import os
import re
import secrets
import shutil
import stat
import struct
import subprocess
import tempfile
from pathlib import Path

from installer.config import CortexOSConfig
from installer.disk import (
    create_partitions,
    device_type,
    format_partition,
    is_block_device,
    mount,
    parent_disk,
    unmount,
)
from installer.ui import ok, info, warn, error

MAGIC, VERSION = b"CRFS", 1
HDR = "<4sIII"
ENT = "<64sII"
HDR_SZ = struct.calcsize(HDR)
ENT_SZ = struct.calcsize(ENT)
USERNAME_RE = re.compile(r"^[a-z_][a-z0-9_-]{0,31}$")
HOSTNAME_RE = re.compile(r"^(?=.{1,253}$)[a-zA-Z0-9](?:[a-zA-Z0-9.-]*[a-zA-Z0-9])?$")


def _build_rootfs(rootfs_dir: str, output: str) -> None:
    """Build CRFS without following symlinks or silently truncating names."""
    root = Path(rootfs_dir).resolve()
    files = []
    for current, dirnames, fnames in os.walk(root, followlinks=False):
        symlink_dirs = [d for d in dirnames if (Path(current) / d).is_symlink()]
        if symlink_dirs:
            raise ValueError(f"El rootfs contiene enlaces simbólicos: {symlink_dirs}")
        dirnames[:] = sorted(dirnames)
        for filename in sorted(fnames):
            path = Path(current) / filename
            if path.is_symlink():
                raise ValueError(f"El rootfs contiene un enlace simbólico: {path.relative_to(root)}")
            name = path.relative_to(root).as_posix()
            encoded = name.encode("utf-8")
            if len(encoded) > 63:
                raise ValueError(f"Nombre demasiado largo para CRFS: {name}")
            files.append((name, path.read_bytes()))

    offset = (HDR_SZ + len(files) * ENT_SZ + 3) & ~3
    entries, content = [], b""
    for name, data in files:
        padded = name.encode("utf-8").ljust(64, b"\x00")
        entries.append(struct.pack(ENT, padded, offset, len(data)))
        padding = (-len(data)) & 3
        content += data + b"\x00" * padding
        offset += len(data) + padding

    Path(output).parent.mkdir(parents=True, exist_ok=True)
    with open(output, "wb") as f:
        f.write(struct.pack(HDR, MAGIC, VERSION, len(files), 0))
        for entry in entries:
            f.write(entry)
        padding = (-f.tell()) & 3
        if padding:
            f.write(b"\x00" * padding)
        f.write(content)


def _password_hash(password: str) -> str:
    """Create a salted PBKDF2 hash for /etc/shadow, never store plaintext."""
    iterations = 310_000
    salt = secrets.token_hex(16)
    digest = hashlib.pbkdf2_hmac(
        "sha256", password.encode("utf-8"), salt.encode("ascii"), iterations
    ).hex()
    return f"$nucleos-pbkdf2-sha256${iterations}${salt}${digest}"


def _validate_config(config: CortexOSConfig) -> None:
    if not HOSTNAME_RE.fullmatch(config.hostname):
        raise ValueError("Hostname inválido: usa letras, números, puntos y guiones")
    if not config.partition:
        raise ValueError("Falta la configuración de particiones")
    if not Path(config.partition.mountpoint).is_absolute() or config.partition.mountpoint == "/":
        raise ValueError("El punto de montaje debe ser absoluto y distinto de /")
    if not config.root_password or any(c in config.root_password for c in ":\n\r"):
        raise ValueError("La contraseña root está vacía o contiene caracteres inválidos")
    for user in config.users or []:
        if not USERNAME_RE.fullmatch(user.username):
            raise ValueError(f"Nombre de usuario inválido: {user.username!r}")
        if not user.password or any(c in user.password for c in ":\n\r"):
            raise ValueError(f"Contraseña inválida para {user.username}")


def _required_commands(config: CortexOSConfig) -> list[str]:
    p = config.partition
    assert p is not None
    commands = ["blkid", "grub-install", "mount", "partprobe", "umount"]
    if p.auto_partition:
        commands += ["mkfs.ext2", "sgdisk", "sfdisk", "wipefs"]
        if p.uefi:
            commands.append("mkfs.fat")
    return commands


def _check_commands(commands: list[str]) -> None:
    missing = [cmd for cmd in commands if not shutil.which(cmd)]
    if missing:
        raise RuntimeError("Faltan dependencias: " + ", ".join(missing))


def _device_uuid(device: str) -> str:
    result = subprocess.run(
        ["blkid", "-o", "value", "-s", "UUID", device],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.stdout.strip()


def _grub_config(boot_device: str, uefi: bool, files_in_boot_root: bool) -> str:
    uuid = _device_uuid(boot_device)
    if uuid:
        root_line = f"search --no-floppy --fs-uuid --set=root {uuid}"
    else:
        # Fallback for unusual filesystems without a UUID.
        root_line = "set root=(hd0,gpt1)" if uefi else "set root=(hd0,msdos1)"

    prefix = "" if files_in_boot_root else "/boot"
    return f"""set timeout=5
set default=0

{root_line}

menuentry \"CortexOS\" {{
    multiboot {prefix}/kernel.bin
    module {prefix}/rootfs.bin rootfs
    boot
}}

menuentry \"Reiniciar\" {{
    reboot
}}

menuentry \"Apagar\" {{
    halt
}}
"""


def _grub_install(target: str, disk: str, boot_device: str, uefi: bool, files_in_boot_root: bool) -> None:
    target = target.rstrip("/")
    boot_dir = str(Path(target) / "boot")
    if not shutil.which("grub-install"):
        raise RuntimeError("grub-install no encontrado")

    common = ["grub-install", f"--boot-directory={boot_dir}", "--recheck"]
    if uefi:
        cmd = [
            *common,
            "--target=x86_64-efi",
            f"--efi-directory={boot_dir}",
            "--bootloader-id=CortexOS",
        ]
        # Avoid requiring firmware NVRAM when installing from a Live USB.
        if not Path("/sys/firmware/efi").is_dir():
            cmd.append("--no-nvram")
        subprocess.run(cmd, check=True, capture_output=True)
    else:
        subprocess.run([*common, "--target=i386-pc", disk], check=True, capture_output=True)

    grub_dir = Path(boot_dir) / "grub"
    grub_dir.mkdir(parents=True, exist_ok=True)
    (grub_dir / "grub.cfg").write_text(
        _grub_config(boot_device, uefi, files_in_boot_root), encoding="utf-8"
    )
    ok("GRUB instalado")


def _configure(target: str, config: CortexOSConfig) -> None:
    etc = Path(target) / "etc"
    etc.mkdir(parents=True, exist_ok=True)

    (etc / "hostname").write_text(f"{config.hostname}\n", encoding="utf-8")
    (etc / "os-release").write_text(
        'NAME="CortexOS"\nVERSION="3.0.0"\nID=cortexos\n'
        'PRETTY_NAME="CortexOS v3"\n'
        'HOME_URL="https://github.com/CRISTOP-bot/nucleos"\n',
        encoding="utf-8",
    )
    (etc / "issue").write_text(
        f"CortexOS v3.0.0 \\n \\l ({config.hostname})\n", encoding="utf-8"
    )

    users = config.users or []
    passwd_lines = ["root:x:0:0:root:/root:/bin/sh"]
    shadow_lines = [f"root:{_password_hash(config.root_password)}:1:0:99999:7:::"]
    used_names = {"root"}
    used_ids = {0}

    for user in users:
        if user.username in used_names:
            raise ValueError(f"Usuario duplicado o reservado: {user.username}")
        uid = 1000
        while uid in used_ids:
            uid += 1
        used_ids.add(uid)
        used_names.add(user.username)
        passwd_lines.append(
            f"{user.username}:x:{uid}:{uid}:{user.username}:/home/{user.username}:/bin/sh"
        )
        shadow_lines.append(f"{user.username}:{_password_hash(user.password)}:1:0:99999:7:::")
        home = Path(target) / "home" / user.username
        home.mkdir(parents=True, exist_ok=True)
        os.chmod(home, 0o700)

    (etc / "passwd").write_text("\n".join(passwd_lines) + "\n", encoding="utf-8")
    shadow = etc / "shadow"
    shadow.write_text("\n".join(shadow_lines) + "\n", encoding="utf-8")
    os.chmod(shadow, 0o600)
    info("Configuración aplicada (contraseñas protegidas en /etc/shadow)")


def _setup(partition) -> dict[str, str]:
    if partition.auto_partition:
        if device_type(partition.device) != "disk":
            raise ValueError(f"El destino debe ser un disco completo: {partition.device}")
        info(f"Particionando {partition.device}...")
        parts = create_partitions(partition.device, partition.uefi)
        boot_part = parts["part0"]
        root_part = parts["part1"]
        format_partition(boot_part, "vfat" if partition.uefi else "ext2", "CORTEXOS_BOOT")
        format_partition(root_part, partition.fstype, "CORTEXOS_ROOT")
        return {"disk": partition.device, "boot": boot_part, "root": root_part}

    disk = partition.disk_device or parent_disk(partition.device)
    if not is_block_device(disk) or device_type(disk) != "disk":
        raise ValueError(f"Disco completo inválido: {disk}")
    if not is_block_device(partition.device) or device_type(partition.device) != "part":
        raise ValueError(f"Partición root inválida: {partition.device}")
    if not is_block_device(partition.boot_device) or device_type(partition.boot_device) != "part":
        raise ValueError(f"Partición boot/EFI inválida: {partition.boot_device}")
    if partition.uefi and partition.boot_device == partition.device:
        raise ValueError("En modo UEFI root y EFI deben ser particiones distintas")

    subprocess.run(["partprobe", disk], capture_output=True, check=True)
    return {"disk": disk, "boot": partition.boot_device, "root": partition.device}


def install(config: CortexOSConfig) -> bool:
    try:
        _validate_config(config)
        _check_commands(_required_commands(config))
    except Exception as exc:
        error(str(exc))
        return False

    p = config.partition
    assert p is not None
    if not is_block_device(p.device) and p.auto_partition:
        error("El dispositivo de destino no es un disco de bloque")
        return False

    mnt = p.mountpoint
    root_mounted = False
    boot_mounted = False
    try:
        parts = _setup(p)

        os.makedirs(mnt, exist_ok=True)
        mount(parts["root"], mnt)
        root_mounted = True

        bmnt = Path(mnt) / "boot"
        bmnt.mkdir(exist_ok=True)
        files_in_boot_root = parts["boot"] != parts["root"]
        if files_in_boot_root:
            mount(parts["boot"], str(bmnt))
            boot_mounted = True

        for directory in ("boot", "dev", "etc", "home", "proc", "sys", "tmp", "root"):
            (Path(mnt) / directory).mkdir(parents=True, exist_ok=True)

        if not os.path.isfile(config.kernel_source):
            raise FileNotFoundError(f"Kernel no encontrado: {config.kernel_source}")
        shutil.copy2(config.kernel_source, bmnt / "kernel.bin")
        ok("Kernel copiado")

        if not os.path.isdir(config.rootfs_source):
            raise FileNotFoundError(f"Rootfs no encontrado: {config.rootfs_source}")

        # Configure a staging copy before packing it. The running kernel reads
        # this CRFS module, not the mounted disk's /etc directory.
        with tempfile.TemporaryDirectory(prefix="nucleos-rootfs-") as staging:
            shutil.copytree(config.rootfs_source, staging, symlinks=True, dirs_exist_ok=True)
            _configure(staging, config)
            _build_rootfs(staging, str(bmnt / "rootfs.bin"))
        ok("Rootfs configurado y empaquetado")

        _grub_install(
            mnt,
            parts["disk"],
            parts["boot"],
            p.uefi,
            files_in_boot_root,
        )
        # Keep the on-disk root consistent with the CRFS module.
        _configure(mnt, config)

        dev = Path(mnt) / "dev"
        dev.mkdir(exist_ok=True)
        try:
            os.mknod(str(dev / "null"), mode=0o666 | stat.S_IFCHR, device=os.makedev(1, 3))
        except OSError:
            warn("No se pudo crear /dev/null; se omitió")

        ok("Instalación completada")
        return True

    except subprocess.CalledProcessError as exc:
        error(f"Falló un comando del instalador: {exc.cmd}")
        if exc.stderr:
            error(exc.stderr.decode(errors="replace")[:500])
        return False
    except Exception as exc:
        error(f"Error: {exc}")
        return False
    finally:
        # Only unmount filesystems mounted by this invocation, boot first.
        if boot_mounted:
            unmount(str(Path(mnt) / "boot"))
        if root_mounted:
            unmount(mnt)
