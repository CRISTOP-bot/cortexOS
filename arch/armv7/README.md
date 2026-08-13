# Port ARMv7 / ARM32 de CortexOS

Este directorio contiene la primera etapa del port ARMv7 para la máquina
`virt` de QEMU y un procesador Cortex-A15:

- entrada ARM en modo privilegiado;
- stack inicial de 32 KiB;
- tabla de excepciones ARMv7 e instalación de `VBAR`;
- consola PL011 en la dirección de QEMU `virt`;
- lectura inicial del DTB, `MIDR` y `SCTLR`;
- linker script con dirección de carga ARM32.

Esta etapa todavía no es el kernel completo de CortexOS. Falta implementar el
parser FDT, MMU de ARMv7, GIC, Generic Timer, PMM, VMM, cambio de contexto,
EL0, syscalls, procesos y drivers.

## Compilar y ejecutar

Se requiere una toolchain ARM32, por ejemplo:

```bash
make armv7-early \
  ARMV7_CC=arm-linux-gnueabihf-gcc \
  ARMV7_LD=arm-linux-gnueabihf-ld
```

Para ejecutar la imagen en QEMU:

```bash
make armv7-run \
  ARMV7_CC=arm-linux-gnueabihf-gcc \
  ARMV7_LD=arm-linux-gnueabihf-ld \
  QEMU_ARMV7=qemu-system-arm
```
