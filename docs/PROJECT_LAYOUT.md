# Organización del proyecto

NucleOS mantiene separado el código fuente, la configuración, las herramientas
y los artefactos generados.

## Reglas

- `kernel/` contiene exclusivamente el kernel y se divide en `core/`,
  `drivers/` y `arch/x86_64/`.
- `config/` contiene archivos fuente de configuración, como el menú de GRUB.
- `rootfs/` es el contenido que se empaqueta dentro del módulo raíz; no debe
  mezclarse con los archivos de compilación.
- `tools/build/` construye rootfs e ISO.
- `tools/installer/` contiene todo el instalador, incluido su launcher.
- `tools/lcp/` contiene el cliente LCP y los repositorios de paquetes.
- `tools/media/` contiene operaciones sobre USB o medios físicos.
- `tools/setup/` contiene instaladores de dependencias del host.
- `user/` contiene la libc y programas que se ejecutarán en modo usuario.
- `third_party/` solo contiene submódulos externos y conserva sus licencias.
- `build/` es temporal y `dist/` contiene artefactos distribuibles; ambos son
  ignorados por Git.

## Flujo de build

```text
kernel/ + kernel/linker.ld ──> build/kernel.bin
rootfs/ + tools/build/rootfs.py ──> build/iso/boot/rootfs.bin
config/grub/ + build/* + tools/installer/ ──> build/iso/
build/iso/ + grub-mkrescue ──> dist/os.iso
```

El `Makefile` es la interfaz principal. Los scripts internos deben recibir sus
rutas como argumentos o calcularlas a partir de su ubicación; no deben asumir
que los artefactos se encuentran en la raíz del repositorio.
