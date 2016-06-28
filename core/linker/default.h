/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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
ENTRY(main_entry)

/* Maximum memory available
 * This is set starting from the minimum memory available from the device
 * family. If for example a family of devices has Flash memories ranging from
 * 512KB to 4MB we must ensure that uVisor fits into 512KB (possibly minus the
 * offset at which uVisor is positioned). */
#define UVISOR_FLASH_LENGTH_AVAIL (FLASH_LENGTH_MIN - FLASH_OFFSET)
#define UVISOR_SRAM_LENGTH_AVAIL  (SRAM_LENGTH_MIN - SRAM_OFFSET)

/* Check that the uVisor memory requirements can be satisfied.
 * Flash: We do not know how much memory uVisor will actually use, so we compare
 *        the available memory against the maximum uVisor could require.
 * SRAM: We already have a symbol of the actual memory space requirement for
 *       uVisor, so we use it for comparison. */
#if UVISOR_FLASH_LENGTH_MAX > UVISOR_FLASH_LENGTH_AVAIL
#error "uVisor does not fit into the target memory. UVISOR_FLASH_LENGTH_MAX must be smaller than UVISOR_FLASH_LENGTH_AVAIL."
#endif /* UVISOR_FLASH_LENGTH > UVISOR_FLASH_LENGTH_AVAIL */
#if UVISOR_SRAM_LENGTH_USED > UVISOR_SRAM_LENGTH_AVAIL
#error "uVisor does not fit into the target memory. UVISOR_SRAM_LENGTH_USED must be smaller than UVISOR_SRAM_LENGTH_AVAIL."
#endif /* UVISOR_SRAM_LENGTH_USED > UVISOR_SRAM_LENGTH_AVAIL */

#ifndef STACK_GUARD_BAND
#define STACK_GUARD_BAND 32
#endif /* STACK_GUARD_BAND */

#ifndef STACK_SIZE
#define STACK_SIZE 1024
#endif /* STACK_SIZE */

MEMORY
{
  FLASH (rx) : ORIGIN = (FLASH_ORIGIN + FLASH_OFFSET),
               LENGTH = UVISOR_FLASH_LENGTH_MAX
  RAM   (rwx): ORIGIN = (SRAM_ORIGIN + SRAM_OFFSET),
               LENGTH = UVISOR_SRAM_LENGTH_USED - STACK_SIZE
  STACK (rw) : ORIGIN = (SRAM_ORIGIN + SRAM_OFFSET + UVISOR_SRAM_LENGTH_USED - STACK_SIZE),
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
        PROVIDE(__uvisor_config = LOADADDR(.export_table) + SIZEOF(.export_table));
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

    .export_table :
    {
        /* The __uvisor_export_table must be placed at the very end of the
         * uVisor binary to work correctly. */
        KEEP(*(.export_table))
    } > FLASH

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
