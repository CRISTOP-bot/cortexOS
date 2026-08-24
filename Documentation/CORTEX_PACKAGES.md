# Paquetes `.cortex`

CortexOS puede leer un contenedor `.cortex` desde el VFS y ejecutar las líneas
`command=` de su manifiesto mediante el shell del kernel.

## Formato v1

Todos los enteros son little-endian:

| Offset | Tamaño | Campo |
|---:|---:|---|
| 0 | 8 | `CORTEXPK` |
| 8 | 2 | versión (`1`) |
| 10 | 4 | tamaño del manifiesto |
| 14 | 4 | tamaño del payload |
| 18 | N | manifiesto UTF-8/ASCII |
| 18+N | M | payload opcional |

El manifiesto contiene una entrada por línea:

```text
name=demo
version=1
command=echo Hola desde CortexOS
command=mkdir /demo
command=ls /demo
```

Las líneas vacías y las que empiezan con `#` se ignoran. Solo `command=` se
ejecuta; los demás metadatos son informativos.

## Límites y seguridad

- manifiesto máximo: 4096 bytes;
- máximo: 32 comandos;
- se valida el magic, la versión y el tamaño total;
- el payload no se ejecuta como código;
- no se permiten comandos externos: deben existir en la tabla del shell.

Uso desde el shell:

```text
pkg run /demo/demo.cortex
```

La creación automática de paquetes desde el host todavía debe implementarse;
por ahora el formato puede generarse con una herramienta externa respetando la
cabecera anterior.
