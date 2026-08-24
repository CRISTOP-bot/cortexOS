# Soporte de Oracle VirtualBox

CortexOS detecta el dispositivo VMMDev de VirtualBox durante la inicialización
PCI. La identificación utilizada es:

```text
Vendor ID: 0x80EE
Device ID: 0xCAFE
```

El driver se encuentra en:

```text
drivers/virtualbox/virtualbox.c
drivers/virtualbox/virtualbox.h
```

En una máquina virtual se puede consultar el estado con:

```text
vbox
```

El driver actualmente informa el bus, slot, función PCI y BAR0. La detección
no implica que Guest Additions esté disponible. Todavía no están implementados
el protocolo VMMDev, clipboard compartido, mouse absoluto, sincronización de
hora, apagado desde el host ni cambio dinámico de resolución.

Para probar la detección, inicia una ISO en VirtualBox con un controlador de
video y red estándar, espera el arranque y ejecuta `vbox` en el shell.
