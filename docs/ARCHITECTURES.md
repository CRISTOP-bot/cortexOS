# Arquitecturas de NucleOS

NucleOS está separando el código común del kernel (`kernel/core/`) del código
específico de cada arquitectura (`kernel/arch/<arquitectura>/`). No se marca
una arquitectura como compatible hasta que pueda compilar, arrancar y pasar un
smoke test.

| Arquitectura | Estado | Arranque | CI | Próximo trabajo |
|---|---|---|---|---|
| x86_64 | Compatible experimental | GRUB Multiboot v1 | Sí | procesos y userspace POSIX |
| i386 / x86 | Preparada | Pendiente | No | GDT/IDT, paging, ABI y linker de 32 bits |
| aarch64 / ARM64 | Boot temprano + MMU/IRQ | QEMU `virt` + UART PL011/GICv2 | Sí | allocator, EL0, syscalls y procesos |
| armv7 / ARM32 | Preparada | Pendiente | No | boot ARM32, MMU, UART, interrupciones y timer |
| riscv64 | Planificada | Pendiente | No | SBI, trap handler, PLIC, timer y paging |

## Selección de arquitectura

El build recibe la arquitectura explícitamente:

```bash
make ARCH=x86_64
make arch-list
make aarch64-early
make aarch64-run
```

Por seguridad, `make ARCH=i386`, `make ARCH=aarch64` y `make ARCH=armv7`
fallan para el kernel completo mientras sus ports no estén completos. AArch64
ya dispone de una imagen independiente de boot temprano que no mezcla los
fuentes x86_64 del kernel principal. No se fabrican kernels que aparenten ser
ARM o x86 de 32 bits cuando todavía usan rutinas x86_64.

## Reglas para un port

Cada port debe aportar, como mínimo:

1. Entrada de boot y salto al punto `kmain`.
2. Linker script y formato de imagen apropiado.
3. Contexto de CPU y cambio de contexto.
4. Interrupciones, excepciones y timer.
5. Consola/UART para diagnóstico.
6. Implementación de `asm.h` o una interfaz de arquitectura equivalente.
7. PMM/VMM adaptados a la MMU de la arquitectura.
8. Perfil CI que compile, arranque en emulador y guarde logs.

El código común no debe incluir instrucciones específicas de x86. Las nuevas
APIs de hardware deben vivir detrás de una interfaz en `kernel/arch/`.

