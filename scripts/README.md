# Scripts de mantenimiento

Los scripts están separados por plataforma para que cada sistema tenga una
entrada clara y no se mezclen comandos de shells diferentes.

```text
scripts/
├── linux/
│   ├── check-layout.py
│   ├── check-python.py
│   ├── verify-crfs.py
│   ├── check-live-ram.py
│   └── nucleos.sh
├── mac/
│   └── nucleos.sh
└── win/
    ├── nucleos.ps1
    ├── nucleos.bat
    ├── run-virtualbox.ps1
    └── run-virtualbox.bat
```

## Linux

```bash
scripts/linux/nucleos.sh check
scripts/linux/nucleos.sh build
scripts/linux/nucleos.sh iso
scripts/linux/nucleos.sh qemu
scripts/linux/nucleos.sh clean

python3 scripts/linux/check-layout.py
python3 scripts/linux/check-python.py
python3 scripts/linux/verify-crfs.py build/iso/boot/rootfs.bin
python3 scripts/linux/check-live-ram.py qemu-serial.log
```

## macOS

```bash
scripts/mac/nucleos.sh check
scripts/mac/nucleos.sh build
scripts/mac/nucleos.sh iso
scripts/mac/nucleos.sh qemu
scripts/mac/nucleos.sh clean
```

El script usa `gmake` cuando está disponible y, en su defecto, `make`. La
creación de la ISO requiere que las herramientas de GRUB, xorriso y mtools
estén disponibles en el entorno elegido.

## Windows

```bat
scripts\win\nucleos.bat check
scripts\win\nucleos.bat build
scripts\win\nucleos.bat iso
scripts\win\nucleos.bat qemu
scripts\win\nucleos.bat virtualbox -Build

scripts\win\run-virtualbox.bat -Build
scripts\win\run-virtualbox.bat -Action status
scripts\win\run-virtualbox.bat -Action stop
```

Los comandos de Windows usan Make nativo cuando existe. Si no está disponible,
buscan WSL y ejecutan el Makefile dentro de la distribución Linux. La VM de
VirtualBox requiere `VBoxManage.exe` y arranca `dist\os.iso` como DVD.
