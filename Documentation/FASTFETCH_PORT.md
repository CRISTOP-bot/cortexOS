# Port de Fastfetch a CortexOS

CortexOS integra el repositorio oficial de Fastfetch como submódulo Git:

```text
https://github.com/fastfetch-cli/fastfetch
```

La fuente está fijada al commit:

```text
a0452b8323aaa9d3b5b6ded435ed6660cee2bbb9
```

Fastfetch conserva su licencia MIT original. CortexOS no copia ni modifica su
fuente dentro del repositorio principal.

## Estado actual

- [x] Submódulo oficial integrado en `third_party/fastfetch/`.
- [x] Commit upstream fijado para builds reproducibles.
- [x] Validación CI de que el submódulo existe y contiene `CMakeLists.txt` y
      `LICENSE`.
- [ ] Implementar las interfaces POSIX que requiere Fastfetch.
- [ ] Añadir una toolchain/sysroot de CortexOS para CMake.
- [ ] Desactivar módulos que dependen de Linux, `/proc`, `/sys`, DBus, Wayland
      u otras APIs ausentes.
- [ ] Compilar un binario estático compatible.
- [ ] Instalarlo como `/bin/fastfetch` dentro del rootfs.

## Por qué no se copia todavía al rootfs

El repositorio oficial es código fuente para sistemas POSIX completos. Copiarlo
como si fuera un ejecutable no funcionaría: CortexOS todavía no ofrece toda la
libc, el ABI POSIX, `/proc`, `/sys`, terminales y APIs necesarias. El comando
`fastfetch` actual del shell es una implementación informativa provisional,
separada del Fastfetch upstream.

Para validar la integración del submódulo:

```bash
make fastfetch-source
git submodule update --init --recursive third_party/fastfetch
```
