# Port AArch64 / ARM64 de NucleOS

Este directorio reserva el código específico del port ARM64.

Todavía no contiene un bootable kernel. El port deberá definir el protocolo de
arranque (UEFI o Device Tree), UART, GIC, timer, MMU, excepciones y contexto de
procesos antes de habilitarse en CI.
