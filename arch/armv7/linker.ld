ENTRY(_start)

SECTIONS
{
	/* QEMU virt loads ARMv7 kernel images at this address. */
	. = 0x40000000;

	.text : ALIGN(32)
	{
		KEEP(*(.text.boot))
		*(.text*)
	}

	.rodata : ALIGN(4)
	{
		*(.rodata*)
	}

	.data : ALIGN(4)
	{
		*(.data*)
	}

	.bss : ALIGN(4)
	{
		__bss_start = .;
		*(.bss*)
		*(COMMON)
		__bss_end = .;
	}

	. = ALIGN(8);
	__stack_bottom = .;
	. += 0x8000;
	__stack_top = .;

	/DISCARD/ :
	{
		*(.comment*)
		*(.note*)
	}
}
