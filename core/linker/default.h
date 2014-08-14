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

MEMORY
{
  FLASH (rx) : ORIGIN = (FLASH_ORIGIN+RESERVED_FLASH), LENGTH = USE_FLASH_SIZE
  RAM (rwx)  : ORIGIN = (SRAM_ORIGIN +RESERVED_SRAM),  LENGTH = USE_SRAM_SIZE
}

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
        . = ALIGN(4);
        __bss_end__ = .;
        __heap_start__ = .;
        *(.heap)
        *(.heap.*)
        . = ALIGN(16);
        __stack_start__ = .;
    } > RAM
}
