# Organización del proyecto

CortexOS sigue una distribución por subsistemas, inspirada en la estructura de
un kernel tradicional. Cada directorio contiene código fuente o documentación
activa; los artefactos generados nunca se guardan en el árbol fuente.

## Directorios principales

- `Documentation/`: arquitectura, ports, ABI y estado del proyecto.
- `arch/`: código específico de arquitectura (`x86_64`, `aarch64`, `armv7` e
  `i386`).
- `kernel/`: código común organizado en `core/`, `apps/`, `console/`,
  `graphics/`, `system/`, `services/` e `include/`.
- `drivers/`: drivers agrupados en `console/`, `input/`, `interrupts/`,
  `pci/` y `serial/`.
- `block/`: `ata/` y `partition/` para almacenamiento de bloques.
- `fs/`: `core/`, `crfs/`, `elf/` y `ext2/`.
- `mm/`: `physical/`, `virtual/` y `heap/`.
- `ipc/`: `process/` y `syscall/`.
- `init/`: `core/` y `openrc/`.
- `net/core/`: red del kernel.
- `lib/`: `core/` para headers internos y `string/` para utilidades de strings.
- `rust/kernel/` y `rust/include/`: módulo Rust y su interfaz C.
- `kernel/core/`: entrada común del kernel y arranque de alto nivel.
- `kernel/apps/`: aplicaciones TUI, calculadora y juegos experimentales.
- `kernel/console/`: consola y shell integrado.
- `kernel/graphics/`: interfaz gráfica experimental.
- `kernel/system/`: GDT, IDT, TSS, persistencia y LCP.
- `kernel/services/`: servicios auxiliares, incluido el instalador provisional.
- `kernel/include/`: headers comunes de entrada y temporizadores.
- `block/`: ATA y particiones.
- `fs/`: VFS, CRFS, ELF y ext2.
- `init/`: inicialización y compatibilidad provisional con OpenRC.
- `ipc/`: procesos y syscalls del kernel x86_64.
- `mm/`: memoria física, memoria virtual y paginación.
- `net/`: red del kernel.
- `lib/`: utilidades internas del kernel.
- `include/`: headers públicos del ABI de usuario.
- `usr/`: crt0, libc, Doom platform boundary, and userspace tests.
- `rust/`: módulo Rust del kernel.
- `rootfs/`: contenido empaquetado en el módulo raíz.
- `config/`: configuración de GRUB, la ISO y el cross-build de OpenRC.
- `config/openrc/`: plantillas Meson para compilar OpenRC contra CortexOS.
- `tools/`: constructor de rootfs, instalador, Archinstall adaptado, LCP,
  medios y setup del host.
- `tools/build/openrc.py`: validación y staging de binarios OpenRC ya compilados.
- `scripts/linux/check-openrc.py`: validación del contrato de fuente y staging.
- `rootfs/etc/conf.d/openrc`: configuración base de OpenRC dentro del rootfs.
- `third_party/`: únicamente submódulos externos y sus licencias, incluido
  el commit fijado de OpenRC y el source-only Doom submodule. No game data.
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
third_party/openrc/ + config/openrc/ + usr/ libc ──> OpenRC cross binaries
OpenRC binaries + tools/build/openrc.py ──> rootfs/sbin/ and rootfs/usr/sbin/
build/iso/ + grub-mkrescue ──> dist/os.iso
```

El `Makefile` es la interfaz principal. Los scripts internos reciben sus rutas
como argumentos o las calculan a partir de su ubicación; no deben asumir que
los artefactos están en la raíz del repositorio.
