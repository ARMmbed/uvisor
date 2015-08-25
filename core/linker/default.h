/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <config.h>

ENTRY(main_entry)

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

MEMORY
{
  FLASH (rx) : ORIGIN = (FLASH_ORIGIN + RESERVED_FLASH),
               LENGTH = USE_FLASH_SIZE - RESERVED_FLASH
  RAM   (rwx): ORIGIN = (SRAM_ORIGIN + RESERVED_SRAM),
               LENGTH = USE_SRAM_SIZE - RESERVED_SRAM - STACK_SIZE
  STACK (rw) : ORIGIN = (SRAM_ORIGIN + USE_SRAM_SIZE - STACK_SIZE),
               LENGTH = STACK_SIZE
}

SECTIONS
{
    .text :
    {
        *(.text.main_entry)
        *(.text*)
        *(.rodata*)
        . = ALIGN(512);
        *(.isr*)
        PROVIDE(__data_start_src__ = LOADADDR(.data));
        PROVIDE(__uvisor_config = LOADADDR(.data) + SIZEOF(.data));
        PROVIDE(__stack_start__ = ORIGIN(STACK));
        PROVIDE(__stack_top__ = ORIGIN(STACK) + LENGTH(STACK) - STACK_GUARD_BAND);
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
        __stack_start__ = .;
    } > RAM
}
