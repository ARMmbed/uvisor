ENTRY(reset_handler)

SECTIONS
{
	.text :
	{
		KEEP(*(.isr_vector_tmp))
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		__etext = .;
	} > FLASH

	.bss :
	{
		. = ALIGN(128);
		*(.isr_vector*)
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss)
		*(.bss.*)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.data : AT (__etext)
	{
		. = ALIGN(4);
		__data_start__ = .;
		__data_start_src__ = __etext;
		*(.ramfunc)
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;
	} > RAM

	.heap :
	{
		. = ALIGN(4);
		__heap_start__ = .;
		*(.heap)
		*(.heap.*)
		. = ALIGN(4);
		__heap_end__ = .;
	} > RAM

	.stack :
	{
		. = ALIGN(4);
		__stack_start__ = .;
		*(.stack)
		*(.stack.*)
		. = ALIGN(4);
		__stack_end__ = .;
	} > RAM
}
