# Archinstall adaptado para NucleOS

Este directorio contiene una copia del paquete funcional de Archinstall y una
capa de adaptación para NucleOS.

## Contenido

- `upstream/archinstall/`: fuentes completas del paquete Python de Archinstall,
  conservadas para reutilizar modelos, TUI, perfiles, traducciones y lógica de
  configuración.
- `upstream/LICENSE`: licencia GPL-3.0 del proyecto original.
- `nucleos.py`: entrada adaptada de NucleOS.

## Diferencia importante

Archinstall está diseñado para Arch Linux y depende de `pacman`, `systemd`,
`arch-chroot`, `pyparted` y otras piezas que no existen en NucleOS. Por eso no
se sustituyeron esas operaciones con comandos falsos. La adaptación usa el
backend existente de NucleOS:

- particionado y montaje controlados por `tools/installer`;
- GRUB para el arranque;
- kernel `build/kernel.bin`;
- rootfs CRFS generado desde `rootfs/`;
- configuración de `/etc`, usuarios y hostname propia de NucleOS.

La copia upstream permanece identificable y separada para conservar sus avisos
legales y facilitar futuras actualizaciones. La interfaz NucleOS no debe
llamarse `archinstall` en una ISO de Arch: su comando es
`nucleos-archinstall`.

## Uso seguro

Primero revisa el plan, que no modifica discos:

```bash
python3 tools/archinstall/nucleos.py \
  --device /dev/sdX \
  --root-password-file /ruta/root.pass \
  --plan
```

La instalación requiere una confirmación explícita adicional:

```bash
sudo python3 tools/archinstall/nucleos.py \
  --device /dev/sdX \
  --root-password-file /ruta/root.pass \
  --install
```

No se debe ejecutar sobre un disco real sin revisar antes el plan y los
nombres de dispositivo.

## Procedencia

Fuente: `archlinux/archinstall`, rama `master`, commit de referencia
`5ebdd27cebbde03705cd11b5c82f0acdaeee9102`, GPL-3.0-only. La copia se importa
como código de terceros y no cambia la licencia de NucleOS.
