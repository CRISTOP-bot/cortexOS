ENTRY(_start)

SECTIONS
{
	/* QEMU virt loads AArch64 kernel images at this address. */
	. = 0x40080000;

	.text : ALIGN(16)
	{
		KEEP(*(.text.boot))
		*(.text*)
	}

	.rodata : ALIGN(16)
	{
		*(.rodata*)
	}

	.data : ALIGN(16)
	{
		*(.data*)
	}

	.bss : ALIGN(16)
	{
		__bss_start = .;
		*(.bss*)
		*(COMMON)
		__bss_end = .;
	}

	. = ALIGN(16);
	__stack_bottom = .;
	. += 0x10000;
	__stack_top = .;

	/DISCARD/ :
	{
		*(.comment*)
		*(.note*)
	}
}
