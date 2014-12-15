/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <config.h>

ENTRY(reset_handler)

#ifndef RESERVED_FLASH
#define RESERVED_FLASH 0x0
#endif/*RESERVED_FLASH*/

#ifndef RESERVED_SRAM
#define RESERVED_SRAM 0x0
#endif/*RESERVED_SRAM*/

#define FLASH_MAX (FLASH_LENGTH-RESERVED_FLASH)
#define SRAM_MAX  (SRAM_LENGTH -RESERVED_SRAM)

#ifdef  USE_FLASH_SIZE
#if   ((USE_FLASH_SIZE) > (FLASH_MAX))
#error "USE_FLASH_SIZE needs to be smaller than FLASH_MAX"
#endif
#else /*USE_FLASH_SIZE*/
#define USE_FLASH_SIZE FLASH_MAX
#endif/*USE_FLASH_SIZE*/

#ifdef  USE_SRAM_SIZE
#if   ((USE_SRAM_SIZE) > (SRAM_MAX))
#error "USE_SRAM_SIZE needs to be smaller than SRAM_MAX"
#endif
#else /*USE_SRAM_SIZE*/
#define USE_SRAM_SIZE SRAM_MAX
#endif/*USE_SRAM_SIZE*/

#ifndef STACK_GUARD_BAND
#define STACK_GUARD_BAND 32
#endif/*STACK_GUARD_BAND*/

#ifndef STACK_SIZE
#define STACK_SIZE 1024
#endif/*STACK_SIZE*/

#define STACK_SECTION_SIZE ((2*STACK_GUARD_BAND)+STACK_SIZE)

MEMORY
{
  FLASH (rx) : ORIGIN = (FLASH_ORIGIN+RESERVED_FLASH), LENGTH = USE_FLASH_SIZE
  RAM   (rwx): ORIGIN = (SRAM_ORIGIN +RESERVED_SRAM),  LENGTH = USE_SRAM_SIZE-STACK_SECTION_SIZE
  STACK (rw) : ORIGIN = (SRAM_ORIGIN +RESERVED_SRAM+USE_SRAM_SIZE-STACK_SECTION_SIZE), LENGTH = STACK_SECTION_SIZE
}

SECTIONS
{
	.text :
	{
		KEEP(*(.box_header))
		KEEP(*(.isr_vector_tmp))
#ifdef  NV_CONFIG_OFFSET
		. = (FLASH_ORIGIN+NV_CONFIG_OFFSET);
		KEEP(*(.nv_config))
#endif/*NV_CONFIG_OFFSET*/
		*(.text.reset_handler)
		*(.svc_vector*)
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
		__box_init_start__ = .;
		KEEP(*(.box_init*))
		__box_init_end__ = .;
		PROVIDE(__data_start_src__ = LOADADDR(.data));
		PROVIDE(__uvisor_config = LOADADDR(.data) + SIZEOF(.data));
		PROVIDE(__stack_end__ = ORIGIN(STACK) + LENGTH(STACK));
		. = ALIGN(16);
		__code_end__ = .;
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

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
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
		__heap_start__ = .;
		*(.heap)
		*(.heap.*)
		__heap_end__ = ALIGN(32);
	} > RAM
}
