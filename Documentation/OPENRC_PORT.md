# OpenRC real en CortexOS

## Estado de esta rama

La fuente oficial de OpenRC se mantiene como submódulo Git:

- Repositorio: `https://github.com/OpenRC/openrc`
- Rama de origen: `master`
- Commit fijado: `04d75bc192486fee932e4e602bdfffc32a0d8b96`
- Ruta: `third_party/openrc/`

La rama conserva un contrato reproducible para la siguiente etapa del port:

- `config/openrc/cortexos-x86_64.ini` es una plantilla Meson que identifica a
  CortexOS como sistema destino y nunca usa el compilador Linux del host.
- `tools/build/openrc.py` valida y copia únicamente los cuatro ejecutables
  OpenRC ya compilados para CortexOS (`openrc-init`, `rc-service`, `rc-status`
  y `rc-update`). Rechaza PIE/ET_DYN, intérpretes dinámicos, arquitecturas
  distintas y segmentos ELF fuera del archivo.
- `make openrc-stage OPENRC_BIN_DIR=...` instala esos binarios en el rootfs,
  pero falla si no existen. No genera binarios falsos ni copia ejecutables del
  host.
- `rootfs/etc/conf.d/openrc` y el árbol de configuración del runlevel default
  están preparados para la instalación de una compilación real.

## Bloqueador x86_64 auditado en esta actualización

El gestor de procesos ya no usa una tabla de páginas global para los procesos
nuevos. Cada proceso recibe un PML4 propio (`cr3`) con el mapeo identidad del
kernel y páginas de usuario separadas para código, stack y heap. `fork()` copia
las páginas de código, stack y heap a un espacio nuevo; el contexto hijo tiene
`rax = 0` y el resultado del padre se registra como el PID del hijo. `execve()`
crea el espacio nuevo, valida y copia sus argumentos desde el espacio de
usuario, carga un ELF estático y solo sustituye el espacio anterior cuando la
carga y el stack inicial terminan correctamente.

El ensamblador x86_64 ahora guarda el frame completo de syscall y de
preempción, restaura los registros generales, cambia `cr3` y vuelve con un
frame `iretq` coherente. Las syscalls que leen o escriben memoria de usuario
validan que todas sus páginas estén presentes, sean de usuario y tengan el
permiso requerido. `sbrk()` también asigna y libera páginas del heap en vez de
mover únicamente un contador.

Esto está implementado y revisado por compilación del código fuente, pero no
se debe presentar como aceptación de OpenRC. La máquina de trabajo local no
trae `make`, el compilador x86_64 bare-metal ni QEMU; el check de GitHub Actions
para este commit sí terminó correctamente con compilación del kernel, validación
Multiboot, ISO arrancable, smoke boot QEMU y ABI/etapas tempranas. Ese smoke
boot no ejecuta todavía el fork/exec de un ELF de usuario ni OpenRC como PID 1,
por lo que esa semántica sigue sin marcarse como runtime-validated.

## Lo que todavía no está implementado o validado

El kernel conserva el gestor de compatibilidad de `init/openrc/openrc.c` para
que la imagen siga siendo utilizable sin un binario real. `init_start_openrc()`
solo intenta el handoff cuando `/sbin/openrc-init` está instalado; esta
condición no convierte al parser de compatibilidad en OpenRC.

Los límites actuales son:

1. **ABI/libc:** faltan la mayoría de interfaces POSIX requeridas por Meson y
   OpenRC, además de errores `errno` completos, usuarios/grupos y soporte
   estático de enlace.
2. **Procesos/ELF:** el cargador todavía acepta solo ELF64 x86_64 `ET_EXEC`
   estático; no hay COW, `PT_INTERP`/PIE, descriptores por proceso heredables,
   ni una prueba runtime de `fork/exec/waitpid`. Las tablas son separadas,
   pero la tabla de descriptores y el VFS siguen siendo globales.
3. **Señales:** no existen `sigaction`, `kill`, `SIGCHLD`, `SIGTERM` ni la
   entrega/reanudación de señales que usa el supervisor.
4. **TTY:** faltan `termios`, `ioctl`, sesiones, grupos de procesos y control
   del terminal.
5. **Sistema:** `/proc` y `/sys` son directorios vacíos, no hay montaje,
   dispositivos reales, reloj/temporizadores POSIX ni soporte suficiente para
   los scripts y utilidades auxiliares de OpenRC.
6. **Rootfs:** la imagen CRFS actual es de solo lectura y no representa
   enlaces simbólicos, permisos, ownership ni runlevels como los espera OpenRC.
7. **Validación:** los checks Python, layout, CRFS y contrato OpenRC pasan
   localmente. GitHub Actions también pasó compilación del kernel, ABI smoke,
   etapas AArch64/ARMv7, validación Multiboot, ISO y smoke boot QEMU; falta una
   prueba específica que ejecute `fork/exec/waitpid` y confirme OpenRC como PID 1.

Completar OpenRC exige resolver esos bloqueadores y añadir una prueba QEMU que
confirme que un binario cross-compilado es PID 1 y arranca/detiene un servicio
real. Esta rama implementa preparación, validación y staging seguros; no
simula esa aceptación.

## Comprobaciones reproducibles

```bash
git submodule update --init --recursive
make check-openrc
make check-layout
make check-python
make verify-crfs
# Solo después de disponer de una libc/toolchain CortexOS:
make openrc-stage OPENRC_BIN_DIR=/ruta/a/openrc-cross/bin
```
