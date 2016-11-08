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

/* Default uVisor stack size
 * Note: This is uVisor own stack, not the boxes' stack. */
#if !defined(STACK_SIZE)
#define STACK_SIZE 2048
#endif /* !defined(STACK_SIZE) */

/* Default uVisor own stack guard band
 * Note: Currently we do not actively use the stack guard to isolate the uVisor
 *       stack from the rest of the protected memories. For this reason the
 *       symbol is arbitrarily small and cannot be defined by the
 *       platform-specific configurations. */
#define STACK_GUARD_BAND 32

MEMORY
{
  FLASH (rx) : ORIGIN = (FLASH_ORIGIN + FLASH_OFFSET),
               LENGTH = UVISOR_FLASH_LENGTH_MAX
  RAM   (rwx): ORIGIN = (SRAM_ORIGIN + SRAM_OFFSET),
               LENGTH = UVISOR_SRAM_LENGTH_USED - STACK_SIZE - STACK_GUARD_BAND
  STACK (rw) : ORIGIN = ORIGIN(RAM) + LENGTH(RAM),
               LENGTH = UVISOR_SRAM_LENGTH_PROTECTED - LENGTH(RAM)
}

SECTIONS
{
    .text :
    {
        KEEP(*(.uvisor_api))
        *(.text*)
        *(.rodata*)
        . = ALIGN(512);
        *(.isr*)
        . = ALIGN(16);
        __code_end__ = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        PROVIDE(__data_start_src__ = LOADADDR(.data));
        __data_start__ = .;
        *(.ramfunc)
        *(.ramfunc.*)
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;
    } > RAM AT > FLASH

    PROVIDE(__uvisor_config = LOADADDR(.data) + SIZEOF(.data));

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    .stack (NOLOAD):
    {
        . = ALIGN(4);
        __stack_start__ = .;
        . += STACK_SIZE;
        __stack_top__ = .;
        . += STACK_GUARD_BAND;
        __stack_end__ = .;
    } > STACK
}
