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
#include "halt.h"
#include "memory_map.h"
#include "svc.h"
#include "unvic.h"
#include "vmpu.h"
#include "vmpu_mpu.h"
#include "page_allocator_faults.h"
#include "page_allocator.h"

/* This file contains the configuration-specific symbols. */
#include "configurations.h"


/* Sanity checks on SRAM boundaries */
#if SRAM_LENGTH_MIN <= 0
#error "SRAM_LENGTH_MIN must be strictly positive."
#endif

static const MpuRegion* vmpu_fault_find_region(uint32_t fault_addr)
{
    const MpuRegion *region;

    /* check current box if not base */
    if ((g_active_box) && ((region = vmpu_region_find_for_address(g_active_box, fault_addr)) != NULL)) {
        return region;
    }

    /* check base-box */
    if ((region = vmpu_region_find_for_address(0, fault_addr)) != NULL) {
        return region;
    }

    /* If no region was found. */
    return NULL;
}

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size)
{
    const MpuRegion *region;

    /* return ACL if available */
    /* FIXME: Use SECURE_ACCESS for SCR! */
    if (fault_addr == (uint32_t) &SCB->SCR) {
        return UVISOR_TACL_UWRITE | UVISOR_TACL_UREAD;
    }

    /* translate fault_addr into its physical address if it is in the bit-banding region */
    if (fault_addr >= VMPU_PERIPH_BITBAND_START && fault_addr <= VMPU_PERIPH_BITBAND_END) {
        fault_addr = VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(fault_addr);
    }
    else if (fault_addr >= VMPU_SRAM_BITBAND_START && fault_addr <= VMPU_SRAM_BITBAND_END) {
        fault_addr = VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(fault_addr);
    }

    /* search base box and active box ACLs */
    if (!(region = vmpu_fault_find_region(fault_addr))) {
        return 0;
    }

    /* ensure that data fits in selected region */
    if((fault_addr + size) > region->end) {
        return 0;
    }

    return region->acl;
}

static int vmpu_mem_push_page_acl_iterator(uint8_t mask, uint8_t index);

static int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status)
{
    const MpuRegion *region;
    uint8_t mask, index, page;

    /* no recovery possible if the MPU syndrome register is not valid */
    if (fault_status != 0x82) {
        return 0;
    }

    if (page_allocator_get_active_mask_for_address(fault_addr, &mask, &index, &page) == UVISOR_ERROR_PAGE_OK)
    {
        /* Remember this fault. */
        page_allocator_register_fault(page);

        vmpu_mem_push_page_acl_iterator(mask, UVISOR_PAGE_MAP_COUNT * 4 - 1 - index);
    }
    else {
        /* find region for faulting address */
        if ((region = vmpu_fault_find_region(fault_addr)) == NULL) {
            return 0;
        }

        vmpu_mpu_push(region, 3);
    }

    return 1;
}

void vmpu_sys_mux_handler(uint32_t lr, uint32_t msp)
{
    uint32_t psp, pc;
    uint32_t fault_addr, fault_status;

    /* the IPSR enumerates interrupt numbers from 0 up, while *_IRQn numbers are
     * both positive (hardware IRQn) and negative (system IRQn); here we convert
     * the IPSR value to this latter encoding */
    int ipsr = ((int) (__get_IPSR() & 0x1FF)) - NVIC_OFFSET;

    /* PSP at fault */
    psp = __get_PSP();

    switch(ipsr)
    {
        case MemoryManagement_IRQn:
            /* currently we only support recovery from unprivileged mode */
            if(lr & 0x4)
            {
                /* pc at fault */
                pc = vmpu_unpriv_uint32_read(psp + (6 * 4));

                /* backup fault address and status */
                fault_addr = SCB->MMFAR;
                fault_status = VMPU_SCB_MMFSR;

                /* check if the fault is an MPU fault */
                if (vmpu_fault_recovery_mpu(pc, psp, fault_addr, fault_status)) {
                    VMPU_SCB_MMFSR = fault_status;
                    return;
                }

                /* if recovery was not successful, throw an error and halt */
                DEBUG_FAULT(FAULT_MEMMANAGE, lr, psp);
                VMPU_SCB_MMFSR = fault_status;
                HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
            }
            else
            {
                DEBUG_FAULT(FAULT_MEMMANAGE, lr, msp);
                HALT_ERROR(FAULT_MEMMANAGE, "Cannot recover from privileged MemManage fault");
            }

        case BusFault_IRQn:
            /* bus faults can be used in a "managed" way, triggered to let uVisor
             * handle some restricted registers
             * note: this feature will not be needed anymore when the register-level
             * will be implemented */

            /* note: all recovery functions update the stacked stack pointer so
             * that exception return points to the correct instruction */

            /* currently we only support recovery from unprivileged mode */
            if(lr & 0x4)
            {
                /* pc at fault */
                pc = vmpu_unpriv_uint32_read(psp + (6 * 4));

                /* backup fault address and status */
                fault_addr = SCB->BFAR;
                fault_status = VMPU_SCB_BFSR;

                /* check if the fault is the special register corner case */
                if (!vmpu_fault_recovery_bus(pc, psp, fault_addr, fault_status)) {
                    VMPU_SCB_BFSR = fault_status;
                    return;
                }

                /* if recovery was not successful, throw an error and halt */
                DEBUG_FAULT(FAULT_BUS, lr, psp);
                HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
            }
            else
            {
                DEBUG_FAULT(FAULT_BUS, lr, msp);
                HALT_ERROR(FAULT_BUS, "Cannot recover from privileged bus fault");
            }
            break;

        case UsageFault_IRQn:
            DEBUG_FAULT(FAULT_USAGE, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_USAGE, "Cannot recover from a usage fault.");
            break;

        case HardFault_IRQn:
            DEBUG_FAULT(FAULT_HARD, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_HARD, "Cannot recover from a hard fault.");
            break;

        case DebugMonitor_IRQn:
            DEBUG_FAULT(FAULT_DEBUG, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_DEBUG, "Cannot recover from a debug fault.");
            break;

        case PendSV_IRQn:
            HALT_ERROR(NOT_IMPLEMENTED, "No PendSV IRQ hook registered");
            break;

        case SysTick_IRQn:
            HALT_ERROR(NOT_IMPLEMENTED, "No SysTick IRQ hook registered");
            break;

        default:
            HALT_ERROR(NOT_ALLOWED, "Active IRQn is not a system interrupt");
            break;
    }
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

/* FIXME: added very simple MPU region switching - optimize! */
void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    uint32_t dst_count;
    const MpuRegion * region;

    /* DPRINTF("switching from %i to %i\n\r", src_box, dst_box); */
    /* Sanity checks */
    if (!vmpu_is_box_id_valid(dst_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL add: The destination box ID is out of range (%u).\r\n", dst_box);
    }

    vmpu_mpu_invalidate();

    /* Update target box first to make target stack available. */
    vmpu_region_get_for_box(dst_box, &region, &dst_count);

    /* Only write stack and context ACL for secure boxes. */
    if (dst_box)
    {
        assert(dst_count);
        /* Push the stack and context protection ACL into ARMv7M_MPU_REGIONS_STATIC. */
        vmpu_mpu_push(region, 255);
        region++;
        dst_count--;
    }

    /* Push one ACL for the page heap into place. */
    page_allocator_iterate_active_page_masks(vmpu_mem_push_page_acl_iterator, PAGE_ALLOCATOR_ITERATOR_DIRECTION_BACKWARD);
    /* g_mpu_slot may now have been incremented by one, if page heap is used by this box. */

    while (dst_count-- && vmpu_mpu_push(region++, 2)) ;

    if (!dst_box)
    {
        /* Handle main box ACLs last. */
        vmpu_region_get_for_box(0, &region, &dst_count);

        while (dst_count-- && vmpu_mpu_push(region++, 1)) ;
    }

}

void vmpu_load_box(uint8_t box_id)
{
    if(box_id == 0)
        vmpu_switch(0, 0);
    else
        HALT_ERROR(NOT_IMPLEMENTED, "currently only box 0 can be loaded");
}

extern int vmpu_region_bits(uint32_t size);

void vmpu_acl_stack(uint8_t box_id, uint32_t bss_size, uint32_t stack_size)
{
    int bits, slots_ctx, slots_stack;
    uint32_t size, block_size;
    static uint32_t box_mem_pos = 0;

    if (box_mem_pos == 0) {
        box_mem_pos = (uint32_t) __uvisor_config.bss_boxes_start;
    }

    /* handle main box */
    if (box_id == 0)
    {
        DPRINTF("ctx=%i stack=%i\n\r", bss_size, stack_size);
        /* non-important sanity checks */
        assert(stack_size == 0);

        /* assign main box stack pointer to existing
         * unprivileged stack pointer */
        g_context_current_states[0].sp = __get_PSP();
        /* Box 0 still uses the main heap to be backwards compatible. */
        g_context_current_states[0].bss = (uint32_t) __uvisor_config.heap_start;
        return;
    }

    /* ensure that box stack is at least UVISOR_MIN_STACK_SIZE */
    stack_size = UVISOR_MIN_STACK(stack_size);

    /* ensure that 2/8th are available for protecting stack from
     * context - include rounding error margin */
    bits = vmpu_region_bits(((stack_size + bss_size) * 8) / 6);

    /* ensure MPU region size of at least 256 bytes for
     * subregion support */
    if(bits < 8) {
        bits = 8;
    }
    size = 1UL << bits;
    block_size = size / 8;

    DPRINTF("\tbox[%i] stack=%i bss=%i rounded=%i\n\r", box_id, stack_size, bss_size, size);

    /* check for correct context address alignment:
     * alignment needs to be a muiltiple of the size */
    if( (box_mem_pos & (size - 1)) != 0 ) {
        box_mem_pos = (box_mem_pos & ~(size - 1)) + size;
    }

    /* check if we have enough memory left */
    if((box_mem_pos + size) > ((uint32_t) __uvisor_config.bss_boxes_end)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "memory overflow - increase uvisor memory allocation\n\r");
    }

    /* round context sizes, leave one free slot */
    slots_ctx = (bss_size + block_size - 1) / block_size;
    slots_stack = slots_ctx ? (8 - slots_ctx - 1) : 8;

    /* final sanity checks */
    if( (slots_ctx * block_size) < bss_size ) {
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_ctx underrun\n\r");
    }
    if( (slots_stack * block_size) < stack_size ) {
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_stack underrun\n\r");
    }

    /* allocate context pointer */
    g_context_current_states[box_id].bss = slots_ctx ? box_mem_pos : (uint32_t) NULL;
    /* ensure stack band on top for stack underflow dectection */
    g_context_current_states[box_id].sp = (box_mem_pos + size - UVISOR_STACK_BAND_SIZE);

    /* Reset uninitialized secured box context. */
    if (slots_ctx) {
        memset((void *) box_mem_pos, 0, bss_size);
    }

    /* create stack protection region */
    size = vmpu_region_add_static_acl(
        box_id,
        box_mem_pos,
        size,
        UVISOR_TACLDEF_STACK,
        slots_ctx ? 1UL << slots_ctx : 0
    );

    /* move on to the next memory block */
    box_mem_pos += size;
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
     * aligned) and has a power-of-two size that is equal or _larger_ than SRAM.
     * This means the region may end _behind_ the end of SRAM!
     *
     * At the beginning of SRAM uVisor places its private BSS section and behind
     * that the page heap. In order to use only one region, we require the end of
     * the page heap to align with 1/8th of the region size, so that we can use
     * the subregion mask.
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
    const uint32_t total_size = (1 << vmpu_region_bits((uint32_t) __uvisor_config.sram_end - (uint32_t) __uvisor_config.sram_start));
    /* The alignment is 1/8th of the region size = rounded up SRAM size. */
    const uint32_t subregions_size = total_size / 8;
    const uint32_t protected_size = (uint32_t) __uvisor_config.page_end - (uint32_t) __uvisor_config.sram_start;
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
        (uint32_t) __uvisor_config.sram_start,
        total_size,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE,
        subregions_disable_mask
    );

    /* On page heap alignments:
     *
     * Individual pages in the page heap are protected by subregions.
     * A page of size 2^N must have its start address aligned to 2^N.
     * However, for page sizes > 1/8th region size, the start address is
     * not guaranteed to be aligned to 2^N.
     *
     * Example: 2^N = page size, 2^(N-1) = 1/8th SRAM (32kB page size in a 128kB SRAM).
     *
     * |           |
     * +-----------+ <-- uvisor_config.page_end: 0x30000 = 3/8th * 128kB is aligned to 16kB.
     * | 32kB page |
     * +-----------+ <-- page start address: 0x30000 - 32kB = 0x10000 is not aligned to 32kB!!
     * |           |
     *
     * Due to these contradicting alignment requirements, it is not possible
     * to have a page size larger than 1/8th region size.
     */
    if (subregions_size < *__uvisor_config.page_size) {
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "The page size (%ukB) must not be larger than 1/8th of SRAM (%ukB).",
                   *__uvisor_config.page_size / 1024, subregions_size / 1024);
    }
}
