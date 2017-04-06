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

#if !defined(SECURE_ALIAS_OFFSET)
#define SECURE_ALIAS_OFFSET 0
#endif /* !defined(SECURE_ALIAS_OFFSET) */

/* Default uVisor non-priviledged stack size for v8-M only. */
#if !defined(STACK_SIZE_NP)
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#define STACK_SIZE_NP 256 /* TODO: choose better default. */
#else
#define STACK_SIZE_NP 0
#endif /* defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U) */
#endif /* !defined(STACK_SIZE_NP) */

/* Default uVisor own stack guard band
 * Note: Currently we do not actively use the stack guard to isolate the uVisor
 *       stack from the rest of the protected memories. For this reason the
 *       symbol is arbitrarily small and cannot be defined by the
 *       platform-specific configurations. */
#define STACK_GUARD_BAND 32

MEMORY
{
  FLASH_NS (r x) : ORIGIN = (FLASH_ORIGIN + FLASH_OFFSET),
                   LENGTH = UVISOR_FLASH_LENGTH_MAX
  FLASH_S  (r x) : ORIGIN = (FLASH_ORIGIN + SECURE_ALIAS_OFFSET + FLASH_OFFSET),
                   LENGTH = UVISOR_FLASH_LENGTH_MAX
  RAM_S    (rwx) : ORIGIN = (SRAM_ORIGIN + SECURE_ALIAS_OFFSET + SRAM_OFFSET),
                   LENGTH = UVISOR_SRAM_LENGTH_USED - (STACK_SIZE + STACK_GUARD_BAND) - (STACK_SIZE_NP ? (STACK_SIZE_NP + STACK_GUARD_BAND) : 0)
  STACK_S  (rw ) : ORIGIN = ORIGIN(RAM_S) + LENGTH(RAM_S),
                   LENGTH = UVISOR_SRAM_LENGTH_PROTECTED - LENGTH(RAM_S)
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
        __uvisor_code_end__ = .;
    } > FLASH_S AT > FLASH_NS

    .ns_text :
    {
        . = ALIGN(32);
        *(.ns_text*)
        . = ALIGN(32);
    } > FLASH_NS

    .data :
    {
        . = ALIGN(4);
        PROVIDE(__uvisor_data_start_src__ = LOADADDR(.data));
        __uvisor_data_start__ = .;
        *(.ramfunc)
        *(.ramfunc.*)
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        /* All data end */
        __uvisor_data_end__ = .;
    } > RAM_S AT > FLASH_NS

#if defined(ARCH_CORE_ARMv8M)
    /* ARMv8-M Entry points section */
    .entry_points :
    {
        . = ALIGN(32);
        __uvisor_entry_points_start__ = .;
        KEEP(*(.entry_points))
        . = ALIGN(32);
        __uvisor_entry_points_end__ = .;
    } > FLASH_NS

    PROVIDE(__uvisor_config = LOADADDR(.entry_points) + SIZEOF(.entry_points));
#else /* defined(ARCH_CORE_ARMv8M) */
    /* The .entry_points section is empty on ARMv7-M. Don't calculate the
     * location of uvisor_config based on entry_points, as even an empty
     * section's ALIGN can change the location counter. */
    PROVIDE(__uvisor_config = LOADADDR(.data) + SIZEOF(.data));
#endif /* defined(ARCH_CORE_ARMv8M) */

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __uvisor_bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __uvisor_bss_end__ = .;
    } > RAM_S

    .stack (NOLOAD):
    {
        . = ALIGN(4);
        /* Non-privileged stack for v8-M only. */
        __uvisor_stack_start_np__ = .;
        . += STACK_SIZE_NP;
        __uvisor_stack_top_np__ = .;
        . += STACK_SIZE_NP ? STACK_GUARD_BAND : 0;
        __uvisor_stack_end_np__ = .;

        /* Privileged stack for v7-M and v8-M. */
        __uvisor_stack_start__ = .;
        . += STACK_SIZE;
        __uvisor_stack_top__ = .;
        . += STACK_GUARD_BAND;
        __uvisor_stack_end__ = .;
    } > STACK_S
}
