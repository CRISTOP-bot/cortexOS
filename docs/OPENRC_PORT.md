# OpenRC real en NucleOS

NucleOS integra la fuente original de OpenRC mediante un submódulo Git:

- Repositorio oficial: `https://github.com/OpenRC/openrc`
- Rama de origen: `master`
- Commit fijado: `04d75bc192486fee932e4e602bdfffc32a0d8b96`
- Ruta: `third_party/openrc/`

Ese commit corresponde a la versión del repositorio observada el 2 de agosto
de 2026, cuyo último cambio visible fue `rc-service: also filter the
environment`.

## Importante

`kernel/core/openrc.c` sigue siendo el gestor provisional de respaldo que NucleOS usa
si no encuentra el binario real. La fuente de `third_party/openrc` es el OpenRC
real y el kernel ya tiene el punto de entrada para entregarle el control como
primer proceso de usuario: busca `/sbin/openrc-init`, le construye
`argv/envp`, lo carga como ELF y habilita el planificador.

Si `/sbin/openrc-init` no existe o no es un ELF válido, NucleOS conserva el
arranque provisional para que la imagen siga siendo utilizable durante el port.
OpenRC todavía necesita libc, procesos, señales, TTY, `/proc`/montajes y varios
servicios POSIX que NucleOS está implementando antes de poder generar ese
binario para NucleOS.

Para comprobar que el submódulo fue descargado:

```bash
git submodule update --init --recursive
make openrc-source
```

El siguiente port debe generar los binarios de OpenRC como programas de
usuario, no copiar sus fuentes al kernel ni reemplazar el código provisional
con una simulación.
