# Port AArch64 / ARM64 de NucleOS

Este directorio contiene la primera etapa de un port para la máquina `virt` de
QEMU y procesadores ARMv8-A:

- entrada `_start` en EL1;
- stack inicial de 64 KiB;
- salida de diagnóstico por UART PL011 (`0x09000000`);
- linker script con dirección de carga para QEMU `virt`;
- bucle de espera mediante `wfe`.

La etapa todavía no es el kernel completo de NucleOS. Aún deben implementarse
la lectura del Device Tree, vectores de excepciones, MMU, allocator, GIC,
timer, drivers, syscalls, procesos y una consola de usuario.

Compilar esta etapa requiere una toolchain AArch64, por ejemplo:

```bash
make aarch64-early \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld
```

Para ejecutarla en QEMU:

```bash
make aarch64-run \
  AARCH64_CC=aarch64-linux-gnu-gcc \
  AARCH64_LD=aarch64-linux-gnu-ld \
  QEMU_AARCH64=qemu-system-aarch64
```
