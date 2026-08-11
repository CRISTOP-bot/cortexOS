# Bash en NucleOS

El código fuente de GNU Bash se incorpora como submódulo Git en
`third_party/bash/`.

El submódulo apunta al espejo público de Bash mantenido por Tianon, que se
identifica como espejo de `git.savannah.gnu.org/cgit/bash.git`. El commit
fijado corresponde a Bash 5.3 y conserva su licencia GPL-3.0.

Después de clonar NucleOS, inicializa el código fuente con:

```sh
git submodule update --init --recursive
```

## Estado actual

El código está **integrado como fuente vendorizado**, pero todavía no se
instala como `/bin/bash`. Compilar Bash para el host no lo convierte en un
programa ejecutable por NucleOS.

Para ejecutar Bash dentro del sistema hacen falta, como mínimo:

- una libc para NucleOS y sus headers;
- un cargador ELF para programas de usuario;
- `fork`, `execve`, `waitpid`, `pipe`, `dup2`, `isatty`, `ioctl`, señales,
  `stat`, `open/read/write` con descriptores reales y manejo de errores POSIX;
- soporte de terminal/TTY y job control;
- un enlazador y una cadena de compilación que produzcan binarios para el ABI
  de NucleOS.

El kernel ya contiene prototipos de algunos syscalls, pero su implementación
actual no proporciona todavía el ABI POSIX/libc que Bash necesita. Por eso no
se agrega un binario falso ni un Bash compilado para Linux al rootfs.

## Validación del vendoring

Desde la raíz del repositorio se puede comprobar que el árbol fuente existe con:

```sh
make bash-source
```

El siguiente paso correcto es desarrollar primero la libc/ABI de usuario y
después añadir un build cross de Bash y el archivo `rootfs/bin/bash` generado.
