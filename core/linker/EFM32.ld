ENTRY(reset_handler)

SECTIONS
{
	.text :
	{
		KEEP(*(.isr_vector_tmp))
		*(.svc_vector*)
		*(.text*)
		*(.rodata*)
		PROVIDE(__data_start_src__ = LOADADDR(.data));
		PROVIDE(__stack_end__ = ORIGIN(RAM) + LENGTH(RAM));
	} > FLASH

	.isr (NOLOAD):
	{
		. = ALIGN(128);
		*(.isr_vector)
	} > RAM

	.data :
	{
		. = ALIGN(4);
		__data_start__ = .;
		*(.ramfunc)
		*(.ramfunc.*)
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;
	} > RAM AT>FLASH

	.bss (NOLOAD):
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss)
		*(.bss.*)
		. = ALIGN(4);
		__bss_end__ = .;
		__heap_start__ = .;
	} > RAM
}
