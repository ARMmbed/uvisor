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
#include <uvisor.h>
#include "debug.h"
#include "context.h"
#include "exc_return.h"
#include "halt.h"
#include "svc.h"
#include "virq.h"
#include "vmpu.h"
#include "vmpu_mpu.h"
#include "page_allocator_faults.h"
#include "page_allocator.h"
#include <stdbool.h>

/* This file contains the configuration-specific symbols. */
#include "configurations.h"

#include <limits.h>

/* Sanity checks on SRAM boundaries */
#if SRAM_LENGTH_MIN <= 0
#error "SRAM_LENGTH_MIN must be strictly positive."
#endif

static const MpuRegion* vmpu_fault_find_region(uint32_t fault_addr)
{
    const MpuRegion *region;

    /* Check current box if not base. */
    if ((g_active_box) && ((region = vmpu_region_find_for_address(g_active_box, fault_addr)) != NULL)) {
        return region;
    }

    /* Check base-box. */
    if ((region = vmpu_region_find_for_address(0, fault_addr)) != NULL) {
        return region;
    }

    /* If no region was found */
    return NULL;
}

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size)
{
    const MpuRegion *region;

    /* Return ACL if available. */
    /* FIXME: Use SECURE_ACCESS for SCR! */
    if (fault_addr == (uint32_t) &SCB->SCR) {
        return UVISOR_TACL_UWRITE | UVISOR_TACL_UREAD;
    }

    /* Translate fault_addr into its physical address if it is in the bit-banding region. */
    if (fault_addr >= VMPU_PERIPH_BITBAND_START && fault_addr <= VMPU_PERIPH_BITBAND_END) {
        fault_addr = VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(fault_addr);
    } else if (fault_addr >= VMPU_SRAM_BITBAND_START && fault_addr <= VMPU_SRAM_BITBAND_END) {
        fault_addr = VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(fault_addr);
    }

    /* Search base box and active box ACLs. */
    if (!(region = vmpu_fault_find_region(fault_addr))) {
        return 0;
    }

    /* Ensure that data fits in selected region. */
    if ((fault_addr + size) > region->end) {
        return 0;
    }

    return region->acl;
}

static int vmpu_mem_push_page_acl_iterator(uint8_t mask, uint8_t index);

static int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status)
{
    const MpuRegion *region;
    uint8_t mask, index, page;

    /* No recovery is possible if the MPU syndrome register is not valid or
     * this is not a stacking fault (where the MPU syndrome register would not
     * be valid, but we can still recover). */
    if (!((fault_status == (SCB_CFSR_MMARVALID_Msk | SCB_CFSR_DACCVIOL_Msk)) ||
          (fault_status & (SCB_CFSR_MSTKERR_Msk | SCB_CFSR_MUNSTKERR_Msk)))) {
        return 0;
    }

    if (page_allocator_get_active_mask_for_address(fault_addr, &mask, &index, &page) == UVISOR_ERROR_PAGE_OK) {
        /* Remember this fault. */
        page_allocator_register_fault(page);

        vmpu_mem_push_page_acl_iterator(mask, UVISOR_PAGE_MAP_COUNT * 4 - 1 - index);
    } else {
        /* Find region for faulting address. */
        if ((region = vmpu_fault_find_region(fault_addr)) == NULL) {
            return 0;
        }

        vmpu_mpu_push(region, 3);
    }

    return 1;
}

uint32_t vmpu_sys_mux_handler(uint32_t lr, uint32_t msp)
{
    uint32_t pc;
    uint32_t fault_addr, fault_status;

    /* The IPSR enumerates interrupt numbers from 0 up, while *_IRQn numbers
     * are both positive (hardware IRQn) and negative (system IRQn); here we
     * convert the IPSR value to this latter encoding. */
    int ipsr = ((int) (__get_IPSR() & 0x1FF)) - NVIC_OFFSET;

    /* Determine the origin of the exception. */
    bool from_psp = EXC_FROM_PSP(lr);
    uint32_t sp = from_psp ? __get_PSP() : msp;

    /* Collect fault information that will be given to the halt handler in case of halt. */
    THaltInfo info, *halt_info = NULL;
    if (debug_collect_halt_info(lr, sp, &info)) {
        halt_info = &info;
    }

    switch (ipsr) {
        case NonMaskableInt_IRQn:
            HALT_ERROR_EXTENDED(NOT_IMPLEMENTED, halt_info, "No NonMaskableInt IRQ handler registered.");
            break;

        case HardFault_IRQn:
            DEBUG_FAULT(FAULT_HARD, lr, sp);
            HALT_ERROR_EXTENDED(FAULT_HARD, halt_info, "Cannot recover from a hard fault.");
            lr = debug_box_enter_from_priv(lr);
            break;

        case MemoryManagement_IRQn:
            fault_status = VMPU_SCB_MMFSR;

            /* If we are having an unstacking fault, we can't read the pc
             * at fault. */
            if (fault_status & (SCB_CFSR_MSTKERR_Msk | SCB_CFSR_MUNSTKERR_Msk)) {
                /* Fake pc */
                pc = 0x0;

                /* The stack pointer is at fault. MMFAR doesn't contain a
                 * valid fault address. */
                fault_addr = sp;
            } else {
                /* pc at fault */
                if (from_psp) {
                    pc = vmpu_unpriv_uint32_read(sp + (6 * 4));
                } else {
                    /* We can be privileged here if we tried doing an ldrt or
                     * strt to a region not currently loaded in the MPU. In
                     * such cases, we are reading from the msp and shouldn't go
                     * through vmpu_unpriv_uint32_read. A box wouldn't have
                     * access to our stack. */
                    pc = *(uint32_t *) (msp + (6 * 4));
                }

                /* Backup fault address and status */
                fault_addr = SCB->MMFAR;
            }

            /* Check if the fault is an MPU fault. */
            if (vmpu_fault_recovery_mpu(pc, sp, fault_addr, fault_status)) {
                VMPU_SCB_MMFSR = fault_status;
                return lr;
            }

            /* If recovery was not successful, throw an error and halt. */
            DEBUG_FAULT(FAULT_MEMMANAGE, lr, sp);
            VMPU_SCB_MMFSR = fault_status;
            HALT_ERROR_EXTENDED(PERMISSION_DENIED, halt_info, "Access to restricted resource denied.");
            lr = debug_box_enter_from_priv(lr);
            break;

        case BusFault_IRQn:
            /* Bus faults can be used in a "managed" way, triggered to let
             * uVisor handle some restricted registers.
             * Note: This feature will not be needed anymore when the
             * register-level will be implemented. */

            /* Note: All recovery functions update the stacked stack pointer so
             * that exception return points to the correct instruction. */

            /* Currently we only support recovery from unprivileged mode. */
            if (from_psp) {
                /* pc at fault */
                pc = vmpu_unpriv_uint32_read(sp + (6 * 4));

                /* Backup fault address and status */
                fault_addr = SCB->BFAR;
                fault_status = VMPU_SCB_BFSR;

                /* Check if the fault is the special register corner case. */
                if (!vmpu_fault_recovery_bus(pc, sp, fault_addr, fault_status)) {
                    VMPU_SCB_BFSR = fault_status;
                    return lr;
                }
            }

            /* If recovery was not successful, throw an error and halt. */
            DEBUG_FAULT(FAULT_BUS, lr, sp);
            HALT_ERROR_EXTENDED(PERMISSION_DENIED, halt_info, "Access to restricted resource denied.");
            break;

        case UsageFault_IRQn:
            DEBUG_FAULT(FAULT_USAGE, lr, sp);
            HALT_ERROR_EXTENDED(FAULT_USAGE, halt_info, "Cannot recover from a usage fault.");
            break;

        case SVCall_IRQn:
            HALT_ERROR_EXTENDED(NOT_IMPLEMENTED, halt_info, "No SVCall IRQ handler registered.");
            break;

        case DebugMonitor_IRQn:
            DEBUG_FAULT(FAULT_DEBUG, lr, sp);
            HALT_ERROR_EXTENDED(FAULT_DEBUG, halt_info, "Cannot recover from a DebugMonitor fault.");
            break;

        case PendSV_IRQn:
            HALT_ERROR_EXTENDED(NOT_IMPLEMENTED, halt_info, "No PendSV IRQ handler registered.");
            break;

        case SysTick_IRQn:
            HALT_ERROR_EXTENDED(NOT_IMPLEMENTED, halt_info, "No SysTick IRQ handler registered.");
            break;

        default:
            HALT_ERROR_EXTENDED(NOT_ALLOWED, halt_info, "Active IRQn (%i) is not a system interrupt.", ipsr);
            break;
    }

    return lr;
}

static int vmpu_mem_push_page_acl_iterator(uint8_t mask, uint8_t index)
{
    MpuRegion region;
    uint32_t size = g_page_size * 8;
    vmpu_region_translate_acl(
        &region,
        (g_page_head_end_rounded - size * (index + 1)),
        size,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE,
        ~mask
    );
    vmpu_mpu_push(&region, 100);
    /* We do not add more than one region for the page heap. */
    return 0;
}

/* This function assumes that its inputs are validated. */
/* FIXME: We've added very simple MPU region switching. - Optimize! */
void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    uint32_t dst_count;
    const MpuRegion * region;

    /* DPRINTF("switching from %i to %i\n\r", src_box, dst_box); */

    vmpu_mpu_invalidate();

    /* Update target box first to make target stack available. */
    vmpu_region_get_for_box(dst_box, &region, &dst_count);

    /* Only write stack and context ACL for secure boxes. */
    if (dst_box) {
        assert(dst_count);
        /* Push the stack and context protection ACL into ARMv7M_MPU_REGIONS_STATIC. */
        vmpu_mpu_push(region, 255);
        region++;
        dst_count--;
    }

    /* Push one ACL for the page heap into place. */
    page_allocator_iterate_active_page_masks(vmpu_mem_push_page_acl_iterator, PAGE_ALLOCATOR_ITERATOR_DIRECTION_BACKWARD);
    /* g_mpu_slot may now have been incremented by one, if page heap is used by this box. */

    while (dst_count-- && vmpu_mpu_push(region++, 2));

    if (!dst_box) {
        /* Handle public box ACLs last. */
        vmpu_region_get_for_box(0, &region, &dst_count);

        while (dst_count-- && vmpu_mpu_push(region++, 1));
    }
}

extern int vmpu_region_bits(uint32_t size);

/* Compute the MPU region size for the given BSS sections and stack sizes.
 * The function also updates the region_start parameter to meet the alignment
 * requirements of the MPU. */
static uint32_t vmpu_acl_sram_region_size(uint32_t * region_start, uint32_t const bss_size, uint32_t const stack_size)
{
    /* Ensure that 2/8th are available for protecting the stack from the BSS
     * sections. One subregion will separate the 2 areas, another one is needed
     * to include a margin for rounding errors. */
    int bits = vmpu_region_bits(((stack_size + bss_size) * 8) / 6);

    /* Calculate the whole MPU region size. */
    /* Note: In order to support subregions the region size is at least 2**8. */
    if (bits < 8) {
        bits = 8;
    }
    uint32_t region_size = 1UL << bits;

    /* Ensure that the MPU region is aligned to a multiple of its size. */
    if ((*region_start & (region_size - 1)) != 0) {
        *region_start = (*region_start & ~(region_size - 1)) + region_size;
    }

    return region_size;
}

void vmpu_acl_sram(uint8_t box_id, uint32_t bss_size, uint32_t stack_size, uint32_t * bss_start,
                   uint32_t * stack_pointer)
{
    /* Offset at which the SRAM region is configured for a secure box. This
     * offset is incremented at every function call. The actual MPU region start
     * address depends on the size and alignment of the region. */
    static uint32_t box_mem_pos = 0;
    if (box_mem_pos == 0) {
        box_mem_pos = (uint32_t) __uvisor_config.bss_boxes_start;
    }

    /* Ensure that box stack is at least UVISOR_MIN_STACK_SIZE. */
    stack_size = UVISOR_MIN_STACK(stack_size);

    /* Compute the MPU region size. */
    /* Note: This function also updates the memory offset to meet the alignment
     *       requirements. */
    uint32_t region_size = vmpu_acl_sram_region_size(&box_mem_pos, bss_size, stack_size);

    /* Allocate the subregions slots for the BSS sections and for the stack.
     * One subregion is used to allow for rounding errors (BSS), and another one
     * is used to separate the BSS sections from the stack. */
    uint32_t subregion_size = region_size / 8;
    int slots_for_bss = (bss_size + subregion_size - 1) / subregion_size;
    int slots_for_stack = slots_for_bss ? (8 - slots_for_bss - 1) : 8;

    /* Final sanity checks */
    if ((slots_for_bss * subregion_size) < bss_size) {
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_ctx underrun\n\r");
    }
    if ((slots_for_stack * subregion_size) < stack_size) {
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_stack underrun\n\r");
    }

    /* Set the pointers to the BSS sections and to the stack. */
    *bss_start = slots_for_bss ? box_mem_pos : (uint32_t) NULL;
    /* `(box_mem_pos + size)` is already outside the memory protected by the
     * MPU region, so a pointer 8B below stack top is chosen (8B due to stack
     * alignment requirements). */
    *stack_pointer = (box_mem_pos + region_size) - 8;

    /* Create stack protection region. */
    region_size = vmpu_region_add_static_acl(
        box_id,
        box_mem_pos,
        region_size,
        UVISOR_TACLDEF_STACK,
        slots_for_bss ? 1UL << slots_for_bss : 0
    );
    DPRINTF("  - SRAM:       0x%08X - 0x%08X (permissions: 0x%04X, subregions: 0x%02X)\r\n",
            box_mem_pos, box_mem_pos + region_size, UVISOR_TACLDEF_STACK, slots_for_bss ? 1UL << slots_for_bss : 0);

    /* Move on to the next memory block. */
    box_mem_pos += region_size;

    DPRINTF("    - BSS:      0x%08X - 0x%08X (original size: %uB, rounded size: %uB)\r\n",
            *bss_start, *bss_start + bss_size, bss_size, slots_for_bss * subregion_size);
    DPRINTF("    - Stack:    0x%08X - 0x%08X (original size: %uB, rounded size: %uB)\r\n",
            *bss_start + (slots_for_bss + 1) * subregion_size, box_mem_pos, stack_size, slots_for_stack * subregion_size);
}

void vmpu_arch_init(void)
{
    /* Init protected box memory enumeration pointer. */
    DPRINTF("\n\rbox stack segment start=0x%08X end=0x%08X (length=%i)\n\r",
        __uvisor_config.bss_boxes_start, __uvisor_config.bss_boxes_end,
        ((uint32_t) __uvisor_config.bss_boxes_end) - ((uint32_t) __uvisor_config.bss_boxes_start));

    vmpu_mpu_init();

    /* Initialize static MPU regions. */
    vmpu_arch_init_hw();

    vmpu_mpu_lock();

    /* Dump MPU configuration in debug mode. */
#ifndef NDEBUG
    debug_mpu_config();
#endif/*NDEBUG*/
}

void vmpu_arch_init_hw(void)
{
    /* Enable the public Flash. */
    vmpu_mpu_set_static_acl(
        0,
        FLASH_ORIGIN,
        ((uint32_t) __uvisor_config.secure_end) - FLASH_ORIGIN,
        UVISOR_TACLDEF_SECURE_CONST | UVISOR_TACL_EXECUTE,
        0
    );

    /* Enable the public SRAM:
     *
     * We use one region for this, which start at SRAM origin (which is always
     * aligned) and has a power-of-two size that is equal or _larger_ than
     * SRAM. This means the region may end _behind_ the end of SRAM!
     *
     * At the beginning of SRAM uVisor places its private BSS section and
     * behind that the page heap. In order to use only one region, we require
     * the end of the page heap to align with 1/8th of the region size, so that
     * we can use the subregion mask.
     * The page heap reduces the memory wastage to less than one page size, by
     * "growing" the page heap downwards from the subregion alignment towards
     * the uVisor bss.
     *
     * Note: The correct alignment needs to be done in the host linkerscript.
     *       Use `ALIGN( (1 << LOG2CEIL(LENGTH(SRAM)) / 8 )` for GNU linker.
     *
     *     2^n     <-- region end
     *     ...
     * .---------. <-- uvisor_config.sram_end
     * |  box 0  |
     * | public  |
     * | memory  |
     * +---------+ <-- uvisor_config.page_end: n/8th of _region_ size (not SRAM size)
     * |  page   |
     * |  heap   |
     * +---------+ <-- aligned to page size
     * | wastage | <-- wasted SRAM is less than 1 page size
     * +---------+ <-- uvisor_config.page_start
     * |  uVisor |
     * |   bss   |
     * '---------' <-- uvisor_config.sram_start, region start
     *
     * Example: The region size of a 96kB SRAM will be 128kB, and the page heap
     *          end will have to be aligned to 16kB, _not_ 12kB (= 96kB / 8).
     *
     * Note: In case the uVisor bss section is moved to another memory region
     *       (tightly-coupled memory for example), the page heap remains and
     *       the same considerations apply. Therefore the uVisor bss section
     *       location has no impact on this.
     */

    /* Calculate the region size by rounding up the SRAM size to the next power-of-two. */
    const uint32_t total_size = (1 << vmpu_region_bits((uint32_t) __uvisor_config.public_sram_end -
                                                       (uint32_t) __uvisor_config.public_sram_start));
    /* The alignment is 1/8th of the region size = rounded up SRAM size. */
    const uint32_t subregions_size = total_size / 8;
    const uint32_t protected_size = (uint32_t) __uvisor_config.page_end - (uint32_t) __uvisor_config.public_sram_start;
    /* The protected size must be aligned to the subregion size. */
    if (protected_size % subregions_size != 0) {
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "The __uvisor_page_end symbol (0x%08X) is not aligned to an MPU subregion boundary.",
                   (uint32_t) __uvisor_config.page_end);
    }
    /* Note: It's called the subregion _disable_ mask, so setting one bit in it _disables_ the
     *       permissions in this subregion. Totally not confusing, amiright!? */
    const uint8_t subregions_disable_mask = (uint8_t) ((1UL << (protected_size / subregions_size)) - 1UL);

    /* Unlock the upper SRAM subregion only. */
    /* Note: We allow code execution for backwards compatibility. Both the user
     *       and superuser flags are set since the ARMv7-M MPU cannot
     *       distinguish between the two. */
    vmpu_mpu_set_static_acl(
        1,
        (uint32_t) __uvisor_config.public_sram_start,
        total_size,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE,
        subregions_disable_mask
    );

    /* On page heap alignments:
     *
     * Individual pages in the page heap are protected by subregions. A page of
     * size 2^N must have its start address aligned to 2^N. However, for page
     * sizes > 1/8th region size, the start address is not guaranteed to be
     * aligned to 2^N.
     *
     * Example: 2^N = page size, 2^(N-1) = 1/8th SRAM (32kB page size in a 128kB SRAM).
     *
     * |           |
     * +-----------+ <-- uvisor_config.page_end: 0x30000 = 3/8th * 128kB is aligned to 16kB.
     * | 32kB page |
     * +-----------+ <-- page start address: 0x30000 - 32kB = 0x10000 is not aligned to 32kB!!
     * |           |
     *
     * Due to these contradicting alignment requirements, it is not possible to
     * have a page size larger than 1/8th region size.
     */
    if (subregions_size < *__uvisor_config.page_size) {
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "The page size (%ukB) must not be larger than 1/8th of SRAM (%ukB).",
                   *__uvisor_config.page_size / 1024, subregions_size / 1024);
    }

    /* Set a static MPU region for uVisor's stack with guard bands
     * protecting stack overflow / underflow.
     */
    vmpu_mpu_set_static_acl(
        2,
        (uint32_t) &__uvisor_stack_start_boundary__,
        (uint32_t) &__uvisor_stack_end__ - (uint32_t) &__uvisor_stack_start_boundary__,
        0,      /* No access */
        0x7E    /* 0b01111110: Sub region disable:
                               * Stack guard on top and on bottom remain with "No access" permissions.
                               * uVisor stack memory inherits access permissions of "Privileged access only" */
    );
}

static void swap_boxes(int * const b1, int * const b2)
{
    uint32_t tmp = *b1;
    *b1 = *b2;
    *b2 = tmp;
}

static uint32_t __vmpu_order_boxes(int * const box_order, int * const best_order, int start, int end, uint32_t min_size)
{
    /* When a permutation is found, calculate the SRAM usage of the box
     * configuration and save the configuration if it's the best so far. */
    if (start == end) {
        uint32_t sram_offset = (uint32_t) __uvisor_config.bss_boxes_start;
        /* Skip the public box. */
        for (int i = 1; i <= end; ++i) {
            int index = box_order[i];
            UvisorBoxConfig const * box_cfgtbl = ((UvisorBoxConfig const * *) __uvisor_config.cfgtbl_ptr_start)[index];
            uint32_t bss_size = 0;
            for (int j = 0; j < UVISOR_BSS_SECTIONS_COUNT; ++j) {
                bss_size += box_cfgtbl->bss.sizes[j];
            }
            uint32_t stack_size = box_cfgtbl->stack_size;
            /* This function automatically updates the sram_offset parameter to
             * meet the MPU alignment requirements. */
            uint32_t region_size = vmpu_acl_sram_region_size(&sram_offset, bss_size, stack_size);
            sram_offset += region_size;
        }
        /* Update the best box configuration. */
        uint32_t sram_size = sram_offset - (uint32_t) __uvisor_config.bss_boxes_start;
        if (sram_size < min_size) {
            min_size = sram_size;
            for (int i = 0; i <= end; ++i) {
                best_order[i] = box_order[i];
            }
        }
    }

    /* Permute the boxes order. */
    for (int i = start; i <= end; ++i) {
        /* Swap, recurse, swap. */
        swap_boxes(&box_order[start], &box_order[i]);
        min_size = __vmpu_order_boxes(box_order, best_order, start + 1, end, min_size);
        swap_boxes(&box_order[start], &box_order[i]);
    }
    return min_size;
}

void vmpu_order_boxes(int * const best_order, int box_count)
{
    /* Start with the same order of configuration table pointers. */
    int box_order[UVISOR_MAX_BOXES];
    for (int i = 0; i < box_count; ++i) {
        box_order[i] = i;
    }
    
    uint32_t total_sram_size = 0;
    if (box_count > 1) {
        /* Find the total amount of SRAM used by all the boxes.
         * This function also updates the best_order array with the configuration
         * that minimizes the SRAM usage. */
        total_sram_size = __vmpu_order_boxes(box_order, best_order, 1, box_count - 1, UINT32_MAX);
    }
    /* This helper message allows people to work around the linker script
     * limitation that prevents us from allocating the correct amount of memory
     * at link time. */
    uint32_t available_sram_size = (uint32_t) __uvisor_config.bss_boxes_end - (uint32_t) __uvisor_config.bss_boxes_start;
    if (available_sram_size < total_sram_size) {
        DPRINTF("Not enough memory allocated for the secure boxes. This is a known limitation of the ARMv7-M MPU.\r\n");
        DPRINTF("Please insert the following snippet in your public box file (usually main.cpp):\r\n");
        DPRINTF("uint8_t __attribute__((section(\".keep.uvisor.bss.boxes\"), aligned(32))) __boxes_overhead[%d];\r\n",
                total_sram_size - available_sram_size);
        HALT_ERROR(SANITY_CHECK_FAILED, "Secure boxes memory overflow. See message above to fix it.\r\n");
    }
}
