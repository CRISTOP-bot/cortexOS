# Scripts de mantenimiento

Estos scripts comprueban y diagnostican el árbol de NucleOS. No sustituyen al
`Makefile`, no escriben en discos y no generan archivos fuente duplicados.

- `check-layout.py`: comprueba las rutas nuevas y detecta rutas obsoletas.
- `check-python.py`: compila los archivos Python para detectar errores de sintaxis.
- `verify-crfs.py`: valida un `rootfs.bin` sin montarlo.
- `check-live-ram.py`: verifica los marcadores del arranque live desde RAM.
- `nucleos.ps1` / `nucleos.bat`: comandos de build, ISO, QEMU y VirtualBox en
  Windows, usando Make nativo o WSL automáticamente.
- `run-virtualbox.ps1` / `run-virtualbox.bat`: crea y arranca una VM de
  VirtualBox con la ISO de NucleOS.

Ejemplos:

```bash
python3 scripts/check-layout.py
python3 scripts/check-python.py
# Con Python 3.14 o superior, comprobar también la copia upstream:
python3 scripts/check-python.py --all
python3 scripts/verify-crfs.py build/iso/boot/rootfs.bin
python3 scripts/check-live-ram.py qemu-serial.log
```

## Windows y VirtualBox

Con VirtualBox instalado y `VBoxManage.exe` disponible:

```bat
scripts\nucleos.bat check
scripts\nucleos.bat iso
scripts\nucleos.bat virtualbox -Build
```

También puedes usar directamente:

```bat
scripts\run-virtualbox.bat -Build
scripts\run-virtualbox.bat -Action status
scripts\run-virtualbox.bat -Action stop
```

Si Windows no tiene `make`, los comandos de build buscan WSL y ejecutan el
Makefile dentro de la distribución Linux. La VM de VirtualBox usa BIOS,
2 CPUs, 1 GB de RAM, red NAT y arranca `dist\os.iso` como DVD.
