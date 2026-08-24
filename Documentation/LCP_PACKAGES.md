# Catálogo de paquetes de LCP

LCP usa un catálogo de texto dentro de `lcp_repo.txt`. Cada bloque describe un
paquete y se separa con `---`:

```text
name:demo
version:1.0.0
description:Paquete de demostración
arch:x86_64
dependencies:libc,termcap
package:/packages/demo.cortex
files:bin/demo
size:4096
repo:main
---
```

`package:` apunta al archivo `.cortex` almacenado en el VFS. `dependencies:`
contiene nombres separados por comas; LCP instala primero cada dependencia,
detecta ciclos y rechaza paquetes cuyo archivo no exista.

## Uso

```text
lcp update
lcp info demo
lcp depends demo
lcp install demo
lcp install --no-deps demo
lcp list --installed
```

Durante `lcp install`, LCP valida el archivo, instala las dependencias y ejecuta
el manifiesto `command=` del paquete. Si el manifiesto falla, el paquete no se
marca como instalado. La base actual es en memoria; la persistencia de la base
de datos y las transacciones de rollback completo quedan pendientes.

## Registro `cortex.org`

El catálogo integrado identifica sus repositorios como `cortex.org/main`,
`cortex.org/games`, `cortex.org/drivers` y `cortex.org/dev`. Si existe
`lcp_repo.txt` en el VFS, ese archivo funciona como espejo local del catálogo y
lo sustituye al ejecutar `lcp update`.

El kernel todavía no implementa un cliente HTTP ni descarga paquetes desde
Internet. Por eso `cortex.org` es actualmente la identidad del registro y su
catálogo local, no una afirmación de que el dominio ya tenga una API compatible.
La sincronización remota necesitará una especificación de API, TLS y validación
criptográfica antes de habilitarse.
