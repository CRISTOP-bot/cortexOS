# Organización del proyecto

NucleOS sigue una distribución por subsistemas, inspirada en la estructura de
un kernel tradicional. Cada directorio contiene código fuente o documentación
activa; los artefactos generados nunca se guardan en el árbol fuente.

## Directorios principales

- `Documentation/`: arquitectura, ports, ABI y estado del proyecto.
- `arch/`: código específico de arquitectura (`x86_64`, `aarch64`, `armv7` e
  `i386`).
- `kernel/`: núcleo común, consola, shell, aplicaciones y gestión del arranque.
- `drivers/`: consola, teclado, mouse, serial, PIC, timer y PCI.
- `block/`: ATA y particiones.
- `fs/`: VFS, CRFS, ELF y ext2.
- `init/`: inicialización y compatibilidad provisional con OpenRC.
- `ipc/`: procesos y syscalls del kernel x86_64.
- `mm/`: memoria física, memoria virtual y paginación.
- `net/`: red del kernel.
- `lib/`: utilidades internas del kernel.
- `include/`: headers públicos del ABI de usuario.
- `usr/`: crt0, libc y pruebas de espacio de usuario.
- `rust/`: módulo Rust del kernel.
- `rootfs/`: contenido empaquetado en el módulo raíz.
- `config/`: configuración de GRUB y de la ISO.
- `tools/`: constructor de rootfs, instalador, LCP, medios y setup del host.
- `third_party/`: únicamente submódulos externos y sus licencias.
- `LICENSES/`: avisos de copyright y licencias del proyecto y sus componentes.
- `certs/`, `crypto/`, `io_uring/`, `samples/`, `scripts/`, `security/`,
  `sound/` y `virt/`: puntos de extensión reservados, con README mientras no
  haya código activo.
- `build/` y `dist/`: temporales e imágenes distribuibles; ambos están
  ignorados por Git.

## Flujo de build

```text
kernel/ + arch/x86_64/ + drivers/ + subsistemas/ ──> build/kernel.bin
rootfs/ + tools/build/rootfs.py ──> build/iso/boot/rootfs.bin
config/grub/ + build/* + tools/installer/ ──> build/iso/
build/iso/ + grub-mkrescue ──> dist/os.iso
```

El `Makefile` es la interfaz principal. Los scripts internos reciben sus rutas
como argumentos o las calculan a partir de su ubicación; no deben asumir que
los artefactos están en la raíz del repositorio.
