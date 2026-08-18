# OpenRC real en CortexOS

## Estado de esta rama

La fuente oficial de OpenRC se mantiene como submódulo Git:

- Repositorio: `https://github.com/OpenRC/openrc`
- Rama de origen: `master`
- Commit fijado: `04d75bc192486fee932e4e602bdfffc32a0d8b96`
- Ruta: `third_party/openrc/`

Esta rama añade un contrato reproducible para la siguiente etapa del port:

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
  ya están preparados para la instalación de una compilación real.

Comprobaciones:

```bash
git submodule update --init --recursive
make openrc-source
make check-openrc
# Solo después de disponer de una libc/toolchain CortexOS:
make openrc-stage OPENRC_BIN_DIR=/ruta/a/openrc-cross/bin
```

## Lo que todavía no está implementado

El kernel conserva el gestor de compatibilidad de `init/openrc/openrc.c` para
que la imagen siga siendo utilizable sin un binario real. `init_start_openrc()`
solo intenta el handoff cuando `/sbin/openrc-init` está instalado; esta
condición no convierte al parser de compatibilidad en OpenRC.

No es honesto afirmar que OpenRC ya arranca como PID 1. El cargador actual
solo admite ELF64 x86_64 `ET_EXEC` estático y el camino de proceso todavía usa
las tablas de páginas globales (`cr3 = 0`), asigna a todos los procesos la
misma ventana de código y no valida punteros de usuario antes de las syscalls.
`fork()` tampoco implementa el retorno 0 en el hijo ni una copia COW/espacio de
direcciones independiente. Por tanto, un ejecutable de OpenRC no puede
considerarse funcional aunque se consiga compilarlo.

Los bloqueadores exactos para ejecutar OpenRC real son:

1. **ABI/libc:** faltan la mayoría de interfaces POSIX requeridas por Meson y
   OpenRC, además de errores `errno` completos, usuarios/grupos y soporte
   estático de enlace.
2. **Procesos/ELF:** falta un espacio de direcciones por proceso, `fork/exec`
   completo, carga de PIE/`PT_INTERP` o una estrategia estática estable, gestión
   de descriptores heredados y semántica fiable de `waitpid`.
3. **Señales:** no existen `sigaction`, `kill`, `SIGCHLD`, `SIGTERM` ni la
   entrega/reanudación de señales que usa el supervisor.
4. **TTY:** faltan `termios`, `ioctl`, sesiones, grupos de procesos y control
   del terminal.
5. **Sistema:** `/proc` y `/sys` son directorios vacíos, no hay montaje,
   dispositivos reales, reloj/temporizadores POSIX ni soporte suficiente para
   los scripts y utilidades auxiliares de OpenRC.
6. **Rootfs:** la imagen CRFS actual es de solo lectura y no representa
   enlaces simbólicos, permisos, ownership ni runlevels como los espera OpenRC.

Completar OpenRC exige resolver esos bloqueadores y añadir una prueba QEMU que
confirme que un binario cross-compilado es PID 1 y arranca/detiene un servicio
real. Esta rama implementa preparación, validación y staging seguros; no
simula esa aceptación.
