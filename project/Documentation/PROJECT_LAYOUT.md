# Organización del proyecto

CortexOS agrupa el árbol fuente en seis categorías de primer nivel. `arch/`,
`Makefile`, `README.txt`, `.github/` y `LICENSE` permanecen en la raíz porque
son la entrada de build, arquitectura, CI y metadatos del repositorio.

## Categorías principales

```text
boot/
├── core/       # init/core: arranque común
└── openrc/     # init/openrc: compatibilidad OpenRC

arch/           # código específico de arquitectura; permanece en la raíz

core/
├── kernel/     # kernel común
├── mm/         # memoria
├── fs/         # VFS, CRFS, ELF y ext2
├── ipc/        # procesos y syscalls
├── security/   # extensiones de seguridad
└── lib/        # utilidades internas del kernel

hw/
├── drivers/    # consola, input, interrupciones, PCI, serial y TTY
├── block/      # ATA y particiones
├── net/        # red
├── sound/      # extensiones de sonido
└── virt/       # extensiones de virtualización

userspace/
├── include/    # headers públicos del ABI
├── usr/        # crt0, libc, Doom y pruebas userspace
├── rootfs/     # contenido empaquetado en CRFS
└── samples/    # muestras de userspace

project/
├── Documentation/
├── scripts/
├── tools/
├── config/
├── third_party/
├── rust/
├── crypto/
├── io_uring/
├── assets/
├── certs/
└── LICENSES/
```

## Integraciones relevantes

- `project/config/`: GRUB, ISO y plantillas cross de OpenRC.
- `project/tools/`: build, rootfs, instalador, medios, Archinstall y LCP.
- `project/tools/build/openrc.py`: validación y staging de binarios OpenRC ya
  compilados.
- `project/scripts/linux/check-layout.py`: valida esta estructura.
- `project/scripts/linux/check-openrc.py`: valida el contrato OpenRC.
- `project/rust/`: módulo Rust del kernel y su interfaz C.
- `project/third_party/`: submódulos externos con sus licencias originales.

## Flujo de build

```text
core/kernel/ + arch/x86_64/ + hw/ + core/ + boot/ ──> build/kernel.bin
userspace/rootfs/ + project/tools/build/rootfs.py ──> build/iso/boot/rootfs.bin
project/config/grub/ + build/* + project/tools/installer/ ──> build/iso/
build/iso/ + grub-mkrescue ──> dist/os.iso
```

El `Makefile` es la interfaz principal. Los scripts calculan la raíz del
repositorio desde su nueva ubicación bajo `project/` y no deben asumir que los
artefactos están junto al script.
