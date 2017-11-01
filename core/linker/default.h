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


#ifdef ARCH_MPU_ARMv7M

/* uVisor stack on MPU_ARMv7M MPU architecture:
*
* .---------------------.    --
* |        Guard        |       \
* |  [STACK_GUARD_BAND] |       |
* +---------------------+       |
* |                     |       |
* |        Stack        |        } => [TOTAL_STACK_SIZE]
* |     [STACK_SIZE]    |       |
* |                     |       |
* +---------------------+       |
* |        Guard        |       |
* |  [STACK_GUARD_BAND] |       /
* '---------------------'    --
*/

/* Default total stack size including guard bands
 * Must be a power of 2 for MPU configurations. */
#if !defined(TOTAL_STACK_SIZE)
#define TOTAL_STACK_SIZE 2048
#endif

/* uVisor own stack guard band
 * Note: Currently we use the stack guard to isolate the uVisor stack on
 *       MPU_ARMv7M MPU architecture only.
 *       In order to use SRD (sub region disabled) we set each guard
 *       (top & bottom) to be 1/8th of the total stack size */
#define STACK_GUARD_BAND (TOTAL_STACK_SIZE >> 3)

/* uVisor stack size
 * Note: This is uVisor own stack, not the boxes' stack. */
#define STACK_SIZE (TOTAL_STACK_SIZE - STACK_GUARD_BAND - STACK_GUARD_BAND)

#else   /* ARCH_MPU_ARMv7M */

/* Default uVisor stack size
 * Note: This is uVisor own stack, not the boxes' stack. */
#if !defined(TOTAL_STACK_SIZE)
#define TOTAL_STACK_SIZE 2080
#endif

/* Default uVisor own stack guard band
 * Note: For MPU architectures other than MPU_ARMv7M, we do not actively use
 *       the stack guard to isolate the uVisor stack from the rest of the
 *       protected memories.
 *       For this reason the symbol is arbitrarily small and cannot be defined
 *       by the platform-specific configurations. */
#define STACK_GUARD_BAND 32

#ifdef ARCH_MPU_KINETIS
#define STACK_SIZE (TOTAL_STACK_SIZE - STACK_GUARD_BAND - STACK_GUARD_BAND)
#else
#define STACK_SIZE (TOTAL_STACK_SIZE - STACK_GUARD_BAND)
#endif

#endif  /* ARCH_MPU_ARMv7M */

#if !defined(SECURE_ALIAS_OFFSET)
#define SECURE_ALIAS_OFFSET 0
#endif /* !defined(SECURE_ALIAS_OFFSET) */

/* Default uVisor non-privileged stack size for v8-M only. */
#if !defined(STACK_SIZE_NP)
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#define STACK_SIZE_NP 256 /* TODO: choose better default. */
#else
#define STACK_SIZE_NP 0
#endif /* defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U) */
#endif /* !defined(STACK_SIZE_NP) */

MEMORY
{
  FLASH_NS (r x) : ORIGIN = (FLASH_ORIGIN + FLASH_OFFSET),
                   LENGTH = UVISOR_FLASH_LENGTH_MAX
  FLASH_S  (r x) : ORIGIN = (FLASH_ORIGIN + SECURE_ALIAS_OFFSET + FLASH_OFFSET),
                   LENGTH = UVISOR_FLASH_LENGTH_MAX
  RAM_S    (rwx) : ORIGIN = (SRAM_ORIGIN + SECURE_ALIAS_OFFSET + SRAM_OFFSET),
                   LENGTH = UVISOR_SRAM_LENGTH_USED - TOTAL_STACK_SIZE - (STACK_SIZE_NP ? (STACK_SIZE_NP + STACK_GUARD_BAND) : 0)
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

#ifdef ARCH_CORE_ARMv8M
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
#else
    /* The .entry_points section is empty on ARMv7-M. Don't calculate the
     * location of uvisor_config based on entry_points, as even an empty
     * section's ALIGN can change the location counter. */
    PROVIDE(__uvisor_config = LOADADDR(.data) + SIZEOF(.data));
#endif

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
#ifdef ARCH_MPU_ARMv7M
        . = ALIGN(TOTAL_STACK_SIZE);
        __uvisor_stack_start_boundary__ = .;
        . += STACK_GUARD_BAND;
#endif
#ifdef ARCH_MPU_KINETIS
    /* Kinetis MPU regions must be 32 byte aligned. */
    . = ALIGN(32);
    __uvisor_stack_start_boundary__ = .;
    . += STACK_GUARD_BAND;
#endif
        __uvisor_stack_start__ = .;
        . += STACK_SIZE;
        __uvisor_stack_top__ = .;
        . += STACK_GUARD_BAND;
        __uvisor_stack_end__ = .;
    } > STACK_S

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM_S
}
