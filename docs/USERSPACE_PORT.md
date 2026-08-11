# Port de espacio de usuario

Este documento marca la primera base para ejecutar programas POSIX pequeños en
NucleOS. No pretende afirmar todavía que Bash sea ejecutable.

## Implementado en esta etapa

- Parser/cargador ELF64 para imágenes `ET_EXEC` x86_64 ya mapeadas.
- Separación de las regiones de código y stack de usuario al crear procesos.
- Retorno de valores de syscalls desde `INT 0x80` hacia `rax`.
- Descriptores VFS para archivos regulares: `open`, `read`, `write`, `close`.
- `dup2` básico para preparar redirecciones.
- `isatty` para stdin/stdout/stderr.
- `exec` deja de copiar bytes arbitrarios y exige una imagen ELF válida.

## Aún necesario para Bash mínimo

- `argc/argv/envp` y una libc estática para NucleOS.
- `fork` con espacios de direcciones realmente independientes.
- `waitpid`, `pipe`, `lseek`, `stat`, `fcntl` y redirecciones completas.
- TTY, señales y grupos de procesos.
- Un área de usuario mayor o mapeo bajo demanda; la ventana actual es solo para
  programas pequeños y no alcanza para Bash.

Cada punto debe tener una prueba de usuario antes de considerarse terminado.
