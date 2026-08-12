# Port i386 / x86 de NucleOS

Este directorio reserva el código específico del port de 32 bits.

Todavía no contiene un bootable kernel. Antes de habilitarlo en el Makefile
hay que adaptar el linker, GDT/IDT, paging, contexto de procesos, ABI y
controladores.
