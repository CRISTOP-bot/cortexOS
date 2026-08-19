# Port de espacio de usuario

Este documento marca la primera base para ejecutar programas POSIX pequeños en
CortexOS. No pretende afirmar todavía que Bash sea ejecutable.

## Implementado en esta etapa

- Parser/cargador ELF64 para imágenes `ET_EXEC` x86_64 ya mapeadas.
- Separación de las regiones de código y stack de usuario al crear procesos.
- Retorno de valores de syscalls desde `INT 0x80` hacia `rax`.
- Descriptores VFS para archivos regulares: `open`, `read`, `write`, `close`.
- `dup2` básico para preparar redirecciones.
- `pipe` y `lseek` básicos.
- `isatty` para stdin/stdout/stderr.
- `stat` con un layout fijo de metadatos para archivos y directorios.
- `fcntl(F_GETFL/F_SETFL)` básico para descriptores.
- `waitpid` con selección de PID y `WNOHANG`.
- `exec` deja de copiar bytes arbitrarios y exige una imagen ELF válida.
- Construcción de `argc/argv/envp` en el stack inicial.
- libc inicial en `userspace/usr/` con wrappers de syscalls, `stdio`, `stdlib`, `string`,
  `malloc/sbrk` y `crt0`.

## Aún necesario para Bash mínimo

- `fork` con espacios de direcciones realmente independientes.
- `waitpid`, `stat`, `fcntl` y redirecciones completas.
- TTY, señales y grupos de procesos.
- Un área de usuario mayor o mapeo bajo demanda; la ventana actual es solo para
  programas pequeños y no alcanza para Bash.

El smoke test de compilación `make user-test-posix` cubre los headers y
wrappers de `stat`, `fcntl` y `waitpid`; todavía falta ejecutar el ELF dentro
de CortexOS y validar el comportamiento con procesos reales. Cada capacidad
debe tener además una prueba de usuario en QEMU antes de considerarse
terminada.
