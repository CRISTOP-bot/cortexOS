# NucleOS

<p align="center">
  <img src="assets/branding/nucleos-logo.png" alt="Logo de NucleOS" width="320">
</p>

![Architecture](https://img.shields.io/badge/architecture-x86__64-blue?style=flat-square)
![Language](https://img.shields.io/badge/language-C%2BASM-orange?style=flat-square)
![Bootloader](https://img.shields.io/badge/boot-GRUB%20Multiboot-purple?style=flat-square)
![License](https://img.shields.io/badge/license-GPLv3-green?style=flat-square)
![Status](https://img.shields.io/badge/status-experimental-red?style=flat-square)

> **Anteriormente conocido como [cris-os-v2](https://github.com/CRISTOP-bot/cris-os-v2).**

**NucleOS** es un sistema operativo modular experimental para **x86_64** que arranca mediante el estándar **Multiboot** con GRUB. Desarrollado en C y ensamblador x86_64, proporciona una base para el estudio de arquitectura de sistemas, gestión de recursos y desarrollo de kernels.

---

## Tabla de Contenidos
- [Características](#características)
- [Arquitectura](#arquitectura)
- [Requisitos](#requisitos)
- [Compilación y Ejecución](#compilación-y-ejecución)
- [Depuración](#depuración)
- [Estructura del Proyecto](#estructura-del-proyecto)
- [Comandos del Shell](#comandos-del-shell)
- [Referencia de Make](#referencia-de-make)
- [Licencia](#licencia)

---

## Características

- **Kernel freestanding x86_64** — 64 bits, sin libc, compilado con `-ffreestanding -m64`.
- **Base multi-arquitectura** — código común separado de `kernel/arch/`, con perfiles preparados para i386, AArch64 y ARMv7; x86_64 es el único port arrancable actualmente.
- **GRUB Multiboot** — Arranque conforme al estándar Multiboot v1.
- **GDT/IDT 64-bit** — Segmentación (L=1 code segments) y tabla de interrupciones con 16-byte entries completamente configuradas.
- **PIC (8259A)** — Remapeo de IRQ y enmascaramiento.
- **PIT (8253)** — Timer programable a 100 Hz.
- **Teclado PS/2** — Controlador con soporte de interrupciones y layout QWERTY/ES/DE intercambiable.
- **Mouse PS/2** — Controlador con aceleración y scroll wheel.
- **Consola VGA** — Modo texto 80×25 con soporte completo de colores (16 colores VGA).
- **Shell interactivo** — Prompt coloreado (`nucleos@nucleos:/path$`), historial, comandos nativos.
- **GNU Bash 5.3 (fuente integrada)** — Incorporado mediante el submódulo `third_party/bash/`; su port como `/bin/bash` queda pendiente de libc, ABI POSIX y cargador ELF para NucleOS.
- **Fastfetch** — Fuente oficial integrada como submódulo en `third_party/fastfetch/`; el binario real queda pendiente del port de libc/POSIX. El shell actual conserva un comando informativo provisional.
- **GUI estilo KDE Plasma** — Escritorio con íconos, panel inferior con reloj y lanzador de aplicaciones.
- **VFS** — Sistema de archivos virtual con soporte para subdirectorios.
- **Rootfs** — Sistema de archivos raíz cargado como módulo Multiboot.
- **OpenRC** — Fuente oficial integrada como submódulo, con handoff del kernel a `/sbin/openrc-init` cuando el binario real esté instalado y fallback provisional durante el port.
- **LCP (Package Manager)** — Gestor de paquetes con repositorios y dependencias.
- **Calculadora** — Evaluador de expresiones aritméticas en C y ensamblador.
- **CPUID** — Detección de vendor, familia y características del procesador.
- **PMM** — Allocador de memoria física con bitmap.
- **VMM** — Gestión de memoria virtual con soporte de mapeo de páginas.
- **Heap** — Allocador de memoria con free-list, splitting y coalescing.
- **PCI** — Enumeración de dispositivos PCI.
- **TUI Apps** — nano (editor), hexview, sysinfo, filemanager, htop, calc-tui.
- **8 Juegos** — Con menú de selección y branding NucleOS.
- **Salida serial** — Debug por puerto serie (COM1, 115200 baud).

---

## Arquitectura

```
┌──────────────────────────────────────────────┐
│              GRUB (Multiboot v1)             │
├──────────────────────────────────────────────┤
│  boot.S — 32→64 long mode trampoline        │
│    Temp GDT → far jump → 64-bit GDT         │
│    PML4 → PDP → PD (2MB pages, 1GB map)     │
├──────────────────────────────────────────────┤
│  kmain() ─── init ─── shell / gui           │
│    ├── gdt_init()    (64-bit GDT)           │
│    ├── idt_init()    (256 entries, 16-byte)  │
│    ├── pic_init() / pic_mask()              │
│    ├── pmm_init()    (bitmap allocator)      │
│    ├── vmm_init()    (identity map)          │
│    ├── memory_init() (heap allocator)        │
│    ├── pci_init()    (device enumeration)    │
│    ├── timer_init()  (100 Hz PIT)           │
│    ├── keyboard_init()                       │
│    ├── mouse_init()                          │
│    ├── boot_init()                           │
│    ├── openrc_init() (service manager)       │
│    └── lcp_init()    (package manager)       │
├──────────────────────────────────────────────┤
│  Shell:  fastfetch, ls, cat, grep, calc     │
│  TUI:    nano, hexview, htop, sysinfo       │
│  Games:  8 juegos con menú                  │
│  VGA:    Colores (16), ASCII art, marcos    │
│  CPUID:  Detección de vendor CPU            │
├──────────────────────────────────────────────┤
│  Drivers:    console, keyboard, mouse       │
│              pic, timer                     │
│  Memory:     PMM (bitmap), VMM, heap        │
│  Bus:        PCI enumeration                │
│  Services:   openrc, lcp, shell             │
│  Utilities:  calc, kstring, cpuid           │
└──────────────────────────────────────────────┘
```

El flujo de arranque comienza en `kernel/arch/x86_64/boot.S` (punto de entrada `start`), que configura un GDT temporal de 32 bits, cambia a modo largo 64 bits via far jump, carga un GDT de 64 bits, establece las page tables de arranque (2MB large pages, 1GB identity map) y salta a `kmain()` en C. `kmain` inicializa los subsistemas en orden, carga el rootfs vía Multiboot y lanza el shell interactivo.

---

## Requisitos

| Herramienta        | Propósito                                     |
|--------------------|-----------------------------------------------|
| `gcc`              | Compilador C para x86_64 (freestanding)       |
| `ld`               | Linker para x86_64                            |
| `make`             | Automatización de build                       |
| `qemu-system-x86_64` | Emulación y pruebas                        |
| `grub-mkrescue`    | Generación de ISO de arranque                 |
| `xorriso`          | Dependencia de `grub-mkrescue`                |
| `python`           | Construcción del rootfs                        |
| `mtools`           | Soporte FAT para `grub-mkrescue`              |
| `grub-install`     | Instalación de GRUB en el disco destino       |
| `sfdisk`, `sgdisk` | Particionado automático                       |
| `wipefs`, `partprobe`, `blkid` | Preparación y detección de discos |
| `mkfs.ext2`, `mkfs.fat` | Formateo de root y EFI                  |

Para crear una ISO, ejecuta `bash tools/setup/install-deps.sh`. Para instalar NucleOS en un disco también necesitas las herramientas de particionado y formateo indicadas arriba (en Debian/Ubuntu suelen estar en `util-linux`, `gdisk`, `parted`, `e2fsprogs` y `dosfstools`). El instalador comprueba las dependencias antes de modificar el disco.

---

## Compilación y Ejecución

```bash
# Instalar dependencias (opcional)
bash tools/setup/install-deps.sh

# Ver arquitecturas y compilar el port actual
make arch-list
make ARCH=x86_64 clean && make ARCH=x86_64

# Generar ISO de arranque
make echo-iso

# Ejecutar con QEMU
qemu-system-x86_64 -cdrom dist/os.iso -m 256M

# Limpiar artefactos de compilación
make clean
```

### Flags de compilación clave

```
-m64                        ← compilación x86_64
-mno-red-zone               ← obligatorio para kernels
-mcmodel=kernel              ← código/kernel en direcciones bajas
-mno-sse -mno-sse2          ← SSE deshabilitado
-mno-mmx -mno-3dnow         ← MMX/3DNow! deshabilitados
-fno-strict-aliasing        ← necesario para -O2
-std=c99                    ← C99 con __asm__ en vez de asm
```

---

## Depuración

```bash
# Con serial output
qemu-system-x86_64 -cdrom dist/os.iso -m 256M -serial stdio

# Con registro de interrupciones
qemu-system-x86_64 -cdrom dist/os.iso -m 256M -serial stdio -d int -no-reboot

# Sin reinicio en triple fault
qemu-system-x86_64 -cdrom dist/os.iso -m 256M -no-reboot
```

---

## Estructura del Proyecto

El repositorio separa código del kernel, arquitectura, herramientas, configuración
y artefactos generados. `build/` y `dist/` nunca forman parte del código fuente.

```text
├── kernel/
│   ├── arch/x86_64/        # Entrada 32→64 y ensamblador específico de x86_64
│   ├── core/               # Kernel, memoria, procesos, VFS, shell y servicios
│   │   └── rust/           # Módulo Rust freestanding
│   ├── drivers/             # Controladores de consola, teclado, mouse, PIC y PIT
│   └── linker.ld            # Script de enlace del kernel
├── config/grub/             # grub.cfg y tema fuente de la ISO
├── rootfs/                  # Contenido empaquetado como módulo Multiboot
├── tools/
│   ├── build/               # Constructor de rootfs
│   ├── installer/           # Instalador Python y su punto de entrada
│   ├── lcp/                 # Cliente LCP y repositorio principal
│   ├── media/               # Creación de USB booteable
│   └── setup/               # Instaladores de dependencias Linux/Windows
├── user/                    # crt0, libc mínima y programas de prueba
├── third_party/             # Submódulos externos sin modificar
│   ├── bash/                # GNU Bash 5.3
│   ├── openrc/              # OpenRC oficial
│   └── fastfetch/           # Fastfetch oficial
├── docs/                    # Documentación técnica y planes de port
│   ├── ARCHITECTURES.md     # Matriz y requisitos multi-arquitectura
│   └── FASTFETCH_PORT.md    # Estado del port de Fastfetch oficial
├── build/                   # Artefactos temporales (ignorado por Git)
├── dist/                    # ISO y sumas de distribución (ignorado por Git)
├── Makefile                 # Puntos de entrada reproducibles del build
└── LICENSE                  # Licencia del código propio
```

Comandos principales:

```bash
make                    # Compilar el kernel
make echo-iso           # Crear dist/os.iso
make run                # Crear la ISO y arrancarla en QEMU
make user-test-hello    # Compilar el primer ELF de usuario
make clean              # Eliminar build/ y dist/
```

## Comandos del Shell

| Comando       | Descripción                                         |
|---------------|-----------------------------------------------------|
| `help`        | Muestra ayuda categorizada                          |
| `fastfetch`   | Información del sistema (CPU, RAM, uptime, etc.)    |
| `gui`         | Inicia la interfaz gráfica estilo KDE Plasma        |
| `ls`          | Lista contenido del directorio                      |
| `pwd`         | Muestra el directorio actual                        |
| `cd`          | Cambia de directorio                                |
| `mkdir`       | Crea un directorio                                  |
| `rmdir`       | Elimina un directorio                               |
| `touch`       | Crea un archivo vacío                               |
| `rm`          | Elimina un archivo                                  |
| `cp`          | Copia un archivo                                    |
| `mv`          | Mueve o renombra un archivo                         |
| `cat`         | Muestra contenido de un archivo                     |
| `grep`        | Busca texto en un archivo                           |
| `echo`        | Imprime texto o escribe a archivo                   |
| `stat`        | Muestra información de un archivo                   |
| `df`          | Muestra uso del sistema de archivos                 |
| `clear`       | Limpia la pantalla                                  |
| `uname`       | Muestra información del sistema (x86_64)            |
| `whoami`      | Muestra el usuario actual                           |
| `kblayout`    | Cambia el layout del teclado (us/es/de)             |
| `mouse`       | Muestra el estado del mouse                         |
| `hexdump`     | Volcado hexadecimal de un archivo                   |
| `wc`          | Cuenta líneas, palabras y caracteres                |
| `head`        | Muestra las primeras líneas de un archivo           |
| `tail`        | Muestra las últimas líneas de un archivo            |
| `calc`        | Calculadora de expresiones                          |
| `asm`         | Operaciones aritméticas en ensamblador              |
| `openrc`      | Gestor de servicios                                 |
| `lcp`         | Gestor de paquetes (29 paquetes, 6 repos)          |
| `games`       | Menú de juegos (8 juegos)                           |
| `nano`        | Editor de texto TUI                                 |
| `hexview`     | Visor hexadecimal TUI                               |
| `sysinfo`     | Panel de información del sistema                    |
| `files`       | Administrador de archivos TUI                       |
| `htop`        | Monitor de procesos                                 |
| `calc-tui`    | Calculadora TUI                                     |
| `reboot`      | Reinicia el sistema                                 |
| `panic`       | Falla el kernel (debug)                             |

---

## Referencia de Make

| Target       | Descripción                                         |
|-------------|-----------------------------------------------------|
| `all`       | Compila el kernel (`build/kernel.bin`)              |
| `iso`       | Compila el kernel + genera rootfs                   |
| `echo-iso`  | `iso` + genera `dist/os.iso`                       |
| `run`       | `echo-iso` + ejecuta con QEMU                       |
| `clean`     | Elimina `build/` y `dist/`                          |

---

## Historial

Este proyecto fue originalmente llamado **CrisOS** (repo: [cris-os-v2](https://github.com/CRISTOP-bot/cris-os-v2)), desarrollado para arquitectura i386. Fue renombrado a **NucleOS** y portado a **x86_64** con soporte completo de modo largo.

---

## Licencia

El código original de NucleOS se distribuye bajo la **GNU General Public License v3.0**.

- Texto legal completo: [LICENSE](LICENSE)
- Resumen y política del proyecto: [LICENSE.md](LICENSE.md)
- Copyright: [COPYRIGHT.md](COPYRIGHT.md)
- Componentes externos y sus licencias: [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)

Bash y OpenRC son submódulos independientes y conservan sus propios avisos,
copyrights y condiciones de distribución.
