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

`src/openrc.c` sigue siendo el gestor provisional que NucleOS usa dentro del
kernel. La fuente de `third_party/openrc` es el OpenRC real, pero todavía no se
compila ni se arranca dentro del kernel: OpenRC necesita un espacio de usuario,
libc, procesos, señales, TTY, `/proc`/montajes y varios servicios POSIX que
NucleOS aún está implementando.

Para comprobar que el submódulo fue descargado:

```bash
git submodule update --init --recursive
make openrc-source
```

El siguiente port debe generar los binarios de OpenRC como programas de
usuario, no copiar sus fuentes al kernel ni reemplazar el código provisional
con una simulación.
