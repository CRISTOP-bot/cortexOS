# Port AArch64 / ARM64 de NucleOS

Este directorio contiene una primera etapa de un port para la máquina `virt` de
QEMU y procesadores ARMv8-A.

## Implementado

- entrada `_start` en EL1;
- stack inicial de 64 KiB;
- tabla de excepciones AArch64 alineada a 2048 bytes;
- instalación de `VBAR_EL1`;
- handler común de excepciones con diagnóstico de `ESR_EL1`, `ELR_EL1` y
  `FAR_EL1`;
- salida por UART PL011 (detectada desde el Device Tree; `0x09000000` en QEMU);
- parser FDT mínimo para RAM, UART PL011 y GICv3;
- lectura de `CurrentEL`, `CNTFRQ_EL0` y `CNTPCT_EL0`;
- MMU con tablas de 4 KiB, identidad inicial de RAM y dispositivos;
- configuración inicial de GICv3 y del Generic Timer PPI;
- IRQ con guardado y restauración del contexto completo de registros;
- diagnóstico de excepciones síncronas mediante `ESR_EL1`, `ELR_EL1` y `FAR_EL1`;
- linker script con dirección de carga para QEMU `virt`;
- bucle de espera mediante `wfe`.

La etapa todavía no es el kernel completo de NucleOS. Aún deben implementarse
un parser FDT completo, allocator basado en los rangos de memoria, paginación
por proceso, drivers completos, syscalls, procesos, EL0 y una consola de
usuario.

## Compilar y ejecutar

Se requiere una toolchain AArch64, por ejemplo:

```bash
make aarch64-early \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld
```

Para ejecutar la imagen en QEMU:

```bash
make aarch64-run \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld \
  QEMU_AARCH64=qemu-system-aarch64
```
