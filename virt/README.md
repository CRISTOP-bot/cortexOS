# virt

Soporte inicial para dispositivos virtuales VirtIO en x86_64.

## Estado actual

- Definiciones compartidas de VirtIO y Virtqueues.
- Acceso a la configuración PCI convencional (`0xCF8`/`0xCFC`).
- Escaneo de buses, slots y funciones PCI.
- Detección de dispositivos VirtIO legacy (`0x1AF4`).
- Validación de BAR0 como I/O BAR.
- Integrado en el `Makefile` y en la secuencia temprana de arranque.

Esto todavía no es un driver funcional de red, consola o almacenamiento: faltan
la negociación de features, la configuración de colas y las operaciones de E/S.
