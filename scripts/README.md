# Scripts de mantenimiento

Estos scripts comprueban y diagnostican el árbol de NucleOS. No sustituyen al
`Makefile`, no escriben en discos y no generan archivos fuente duplicados.

- `check-layout.py`: comprueba las rutas nuevas y detecta rutas obsoletas.
- `check-python.py`: compila los archivos Python para detectar errores de sintaxis.
- `verify-crfs.py`: valida un `rootfs.bin` sin montarlo.
- `check-live-ram.py`: verifica los marcadores del arranque live desde RAM.

Ejemplos:

```bash
python3 scripts/check-layout.py
python3 scripts/check-python.py
# Con Python 3.14 o superior, comprobar también la copia upstream:
python3 scripts/check-python.py --all
python3 scripts/verify-crfs.py build/iso/boot/rootfs.bin
python3 scripts/check-live-ram.py qemu-serial.log
```
