# Arranque live desde USB con rootfs en RAM

La entrada normal de GRUB carga `kernel.bin` y `rootfs.bin` como módulos
Multiboot. GRUB los copia a memoria antes de entregar el control al kernel.
Durante el arranque, CortexOS reserva el rango del módulo para que el PMM no lo
reutilice y copia el CRFS a páginas propias del PMM.

El arranque muestra:

```text
Rootfs copied to RAM
Initialized VFS (RAM rootfs)
```

Después de esos mensajes, el sistema usa el kernel y el rootfs desde RAM y ya
no necesita leer el USB para el funcionamiento normal. El USB puede retirarse
cuando el mensaje de rootfs en RAM haya aparecido y el sistema haya terminado
de inicializar el VFS.

## Importante

- No retires el USB durante GRUB ni antes de que aparezca el mensaje.
- Esto aplica al arranque live de la ISO con el módulo Multiboot.
- Los datos que se escriban en el rootfs live son temporales y se pierden al
  apagar o reiniciar, salvo que se copien explícitamente a un disco.
- El instalador sigue necesitando el medio y el disco destino hasta completar
  la instalación.

El código de reserva y copia está en `mm/physical/pmm.c` y `kernel/core/kernel.c`.
