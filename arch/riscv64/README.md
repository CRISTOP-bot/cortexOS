# RISC-V 64 port

This port provides an independent early boot image for QEMU `virt`. It starts
under OpenSBI, initializes the 16550-compatible UART at `0x10000000`, and
prints a smoke-test marker. It does not yet enter the common kernel or provide
paging, traps, PLIC, timer, processes, or a root filesystem.
