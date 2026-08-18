# Plan de implementación por etapas

El objetivo es ejecutar primero programas POSIX pequeños y después arrancar
OpenRC y Bash reales. Cada etapa debe compilar y tener una prueba antes de
pasar a la siguiente.

## Etapa 1 — ABI y primer ELF

- [x] Cargador ELF64 `ET_EXEC`.
- [x] Stack inicial con `argc/argv/envp`.
- [x] Wrappers de libc y `crt0`.
- [x] `open/read/write/close/dup2/pipe/lseek` iniciales.
- [ ] Compilar y arrancar un programa `hello.elf` dentro del rootfs.
- [x] Validar rangos y permisos de punteros de usuario antes de las syscalls
      que acceden a memoria.

**Aceptación:** un ELF estático escribe en stdout, lee un archivo y termina
con un código observable por `waitpid()`. Sigue pendiente la ejecución real.

## Etapa 2 — Procesos y memoria

- [x] Crear un espacio de direcciones por proceso (`cr3` propio) con mapeo de
      kernel y páginas de usuario separadas.
- [x] Guardar/restaurar el frame completo de syscall/interrupción, registros,
      `cr3` y retorno `iretq` en x86_64.
- [x] Hacer que el modelo de `fork()` prepare retorno 0 en el hijo y PID en el
      padre, copiando código, stack y heap a páginas independientes.
- [ ] Heredar y cerrar descriptores correctamente: la tabla de FDs y el VFS
      todavía son globales.
- [x] Completar la sustitución de espacio en `execve()` para ELF estático y
      copiar de forma segura `path/argv/envp`.
- [x] Hacer que `sbrk()` asigne/libere páginas del heap.
- [ ] Ejecutar y observar `fork()`, `execve()` y `waitpid()` bajo QEMU.

**Estado exacto:** la implementación de tablas, contexto y validación está en
el código; la aceptación runtime no está demostrada. Sigue faltando COW,
`PIE/PT_INTERP`, FDs por proceso y la prueba QEMU.

## Etapa 3 — POSIX de archivos y libc

- [ ] `stat/fstat`, `fcntl`, `access`, `unlink`, `mkdir` y `getdents`.
- [ ] Pipes bloqueantes, EOF y `SIGPIPE`.
- [ ] `fopen/fread/fwrite/fseek`, `calloc/realloc`, errores y tiempo.
- [ ] Escritura persistente sobre ext2.

**Aceptación:** funcionan `cat`, redirecciones y pipelines de varios comandos.

## Etapa 4 — TTY y señales

- [ ] `termios`, `ioctl`, `tcgetattr` y `tcsetattr`.
- [ ] `SIGINT`, `SIGTERM`, `SIGCHLD`, `SIGTSTP` y `kill`.
- [ ] Sesiones, grupos de procesos y control de trabajos.

**Aceptación:** un shell interactivo responde a Ctrl+C/Ctrl+Z y soporta
`jobs`, `fg` y `bg`.

## Etapa 5 — OpenRC real

- [x] Fuente oficial fijada como submódulo.
- [x] Punto de handoff condicionado a `/sbin/openrc-init` y cargador ELF.
- [x] Plantilla Meson cross para CortexOS y validador de staging seguro.
- [ ] Completar libc/ABI POSIX y compilar OpenRC estáticamente contra ella.
- [ ] Instalar binarios cross-compilados: `openrc-init`, `rc-service`,
      `rc-status` y `rc-update`.
- [ ] Implementar espacios de direcciones, señales, TTY, montajes y runlevels.
      El espacio de direcciones x86_64 ya está separado, pero el resto sigue
      pendiente.
- [ ] Probar un servicio real bajo QEMU como PID 1.

**Aceptación:** OpenRC es PID 1 y arranca/detiene un servicio real. La
existencia del punto de handoff, el submódulo o las tablas de páginas no cuenta
como aceptación.

## Etapa 6 — Bash real

- [ ] Completar las interfaces detectadas por `configure`.
- [ ] Compilar Bash estático contra la libc de CortexOS.
- [ ] Instalarlo como `/bin/bash` y usarlo como shell por defecto.

**Aceptación:** Bash ejecuta comandos, scripts, pipes, redirecciones y
programas externos dentro de CortexOS.
