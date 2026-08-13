# Arquitecturas de CortexOS

CortexOS está separando el código común del kernel (`kernel/`) del código
específico de cada arquitectura (`arch/<arquitectura>/`). No se marca
una arquitectura como compatible hasta que pueda compilar, arrancar y pasar un
smoke test.

| Arquitectura | Estado | Arranque | CI | Próximo trabajo |
|---|---|---|---|---|
| x86_64 | Compatible experimental | GRUB Multiboot v1 | Sí | procesos y userspace POSIX |
| i386 / x86 | Preparada | Pendiente | No | GDT/IDT, paging, ABI y linker de 32 bits |
| aarch64 / ARM64 | Boot + PMM/EL0/SVC smoke | QEMU `virt` + UART PL011/GICv2 | Sí | aislamiento por proceso, scheduler y rootfs |
| armv7 / ARM32 | Boot temprano | QEMU `virt` + UART PL011 | Sí | FDT, MMU, GIC, timer y EL0 |
| riscv64 | Planificada | Pendiente | No | SBI, trap handler, PLIC, timer y paging |

## Selección de arquitectura

El build recibe la arquitectura explícitamente:

```bash
make ARCH=x86_64
make arch-list
make aarch64-early
make aarch64-run
make armv7-early
make armv7-run
```

Por seguridad, `make ARCH=i386`, `make ARCH=aarch64` y `make ARCH=armv7`
fallan para el kernel completo mientras sus ports no estén completos. AArch64
y ARMv7 ya disponen de imágenes independientes con smoke tests de arranque que
no mezclan los fuentes x86_64 del kernel principal. AArch64 valida además PMM,
EL0 y SVC, pero todavía no declara aislamiento por proceso ni rootfs integrado.
No se fabrican kernels que
aparenten estar completos cuando todavía usan rutinas de otra arquitectura.

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
APIs de hardware deben vivir detrás de una interfaz en `arch/`.

