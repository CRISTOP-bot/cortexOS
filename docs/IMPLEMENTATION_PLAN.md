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
- [ ] Validar punteros de usuario antes de cada syscall.

**Aceptación:** un ELF estático escribe en stdout, lee un archivo y termina
con un código observable por `waitpid()`.

## Etapa 2 — Procesos y memoria

- [ ] Crear un espacio de direcciones por proceso.
- [ ] Guardar/restaurar contexto completo al entrar y salir de una syscall.
- [ ] Hacer que `fork()` retorne 0 al hijo y el PID al padre.
- [ ] Heredar y cerrar descriptores correctamente.
- [ ] Completar `execve()` y limpiar el espacio anterior.

**Aceptación:** `fork()`, `execve()` y `waitpid()` ejecutan dos procesos sin
compartir accidentalmente su stack o código.

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
- [x] Handoff del kernel a `/sbin/openrc-init`.
- [ ] Compilar OpenRC estáticamente contra la libc de NucleOS.
- [ ] Instalar `openrc-init`, `rc-service`, `rc-status` y `rc-update`.
- [ ] Implementar runlevels y servicios en `/etc/init.d`.

**Aceptación:** OpenRC es PID 1 y arranca/detiene un servicio real.

## Etapa 6 — Bash real

- [ ] Completar las interfaces detectadas por `configure`.
- [ ] Compilar Bash estático contra la libc de NucleOS.
- [ ] Instalarlo como `/bin/bash` y usarlo como shell por defecto.

**Aceptación:** Bash ejecuta comandos, scripts, pipes, redirecciones y
programas externos dentro de NucleOS.
