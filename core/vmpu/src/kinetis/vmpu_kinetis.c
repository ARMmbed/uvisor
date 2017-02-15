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
#include "vmpu_kinetis.h"
#include "vmpu_kinetis_aips.h"
#include "vmpu_kinetis_mem.h"
#include "page_allocator_faults.h"

uint32_t g_box_mem_pos;

static int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp, uint32_t fault_addr)
{
    uint32_t start_addr, end_addr;
    uint8_t page;

    /* Check if the fault address is a page. */
    if (page_allocator_get_active_region_for_address(fault_addr, &start_addr, &end_addr, &page) == UVISOR_ERROR_PAGE_OK)
    {
        /* Remember this fault. */
        page_allocator_register_fault(page);
        DPRINTF("Page Fault for address 0x%08x at page %u [0x%08x, 0x%08x]\n", fault_addr, page, start_addr, end_addr);
        /* Create a page ACL for this page and enable it. */
        if (vmpu_mem_push_page_acl(start_addr, end_addr)) {
            return -1;
        }
        return 0;
    }
    return -1;
}

uint32_t vmpu_sys_mux_handler(uint32_t lr, uint32_t msp)
{
    uint32_t psp, pc;
    uint32_t fault_addr, fault_status;

    /* The IPSR enumerates interrupt numbers from 0 up, while *_IRQn numbers
     * are both positive (hardware IRQn) and negative (system IRQn); here we
     * convert the IPSR value to this latter encoding */
    int ipsr = ((int) (__get_IPSR() & 0x1FF)) - NVIC_OFFSET;

    /* PSP at fault */
    psp = __get_PSP();

    switch (ipsr) {
        case MemoryManagement_IRQn:
            DEBUG_FAULT(FAULT_MEMMANAGE, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_MEMMANAGE, "Cannot recover from a memmanage fault");
            break;

        case BusFault_IRQn:
            /* Where a Freescale MPU is used, bus faults can originate both as
             * pure bus faults or as MPU faults; in addition, they can be both
             * precise and imprecise.
             * There is also an additional corner case: some registers (MK64F)
             * cannot be accessed in unprivileged mode even if an MPU region is
             * created for them and the corresponding bit in PACRx is set. In
             * some cases we want to allow access for them (with ACLs), hence
             * we use a function that looks for a specially crafted opcode. */

            /* Note: All recovery functions update the stacked stack pointer so
             * that exception return points to the correct instruction. */

            fault_status = VMPU_SCB_BFSR;

            /* If we are having an unstacking fault, we can't read the pc
             * at fault. */
            if (fault_status & (SCB_CFSR_MSTKERR_Msk | SCB_CFSR_MUNSTKERR_Msk)) {
                /* fake pc */
                pc = 0x0;

                /* The stack pointer is at fault. BFAR doesn't contain a
                 * valid fault address. */
                fault_addr = psp;
            } else {
                /* pc at fault */
                if (lr & 0x4) {
                    pc = vmpu_unpriv_uint32_read(psp + (6 * 4));
                } else {
                    /* We can be privileged here if we tried doing an ldrt or
                     * strt to a region not currently loaded in the MPU. In
                     * such cases, we are reading from the msp and shouldn't go
                     * through vmpu_unpriv_uint32_read. A box wouldn't have
                     * access to our stack. */
                    pc = *(uint32_t *) (msp + (6 * 4));
                }

                /* backup fault address and status */
                fault_addr = SCB->BFAR;
            }

            /* Check if the fault is an MPU fault. */
            int slave_port = vmpu_fault_get_slave_port();
            if (slave_port >= 0) {
                /* If the fault comes from the MPU module, we don't use the
                 * bus fault syndrome register, but the MPU one. */
                fault_addr = MPU->SP[slave_port].EAR;

                /* Check if we can recover from the MPU fault. */
                if (!vmpu_fault_recovery_mpu(pc, psp, fault_addr)) {
                    /* We clear the bus fault status anyway. */
                    VMPU_SCB_BFSR = fault_status;

                    /* We also clear the MPU fault status bit. */
                    vmpu_fault_clear_slave_port(slave_port);

                    /* Recover from the exception. */
                    return lr;
                }
            } else if (slave_port == VMPU_FAULT_MULTIPLE) {
                DPRINTF("Multiple MPU violations found.\r\n");
            }

            /* Check if the fault is the special register corner case. */
            if (!vmpu_fault_recovery_bus(pc, psp, fault_addr, fault_status)) {
                VMPU_SCB_BFSR = fault_status;
                return lr;
            }

            /* If recovery was not successful, throw an error and halt. */
            DEBUG_FAULT(FAULT_BUS, lr, lr & 0x4 ? psp : msp);
            VMPU_SCB_BFSR = fault_status;
            HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
            lr = debug_box_enter_from_priv(lr);
            break;

        case UsageFault_IRQn:
            DEBUG_FAULT(FAULT_USAGE, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_USAGE, "Cannot recover from a usage fault.");
            break;

        case HardFault_IRQn:
            DEBUG_FAULT(FAULT_HARD, lr, lr & 0x4 ? psp : msp);
            HALT_ERROR(FAULT_HARD, "Cannot recover from a hard fault.");
            lr = debug_box_enter_from_priv(lr);
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
            HALT_ERROR(NOT_ALLOWED, "Active IRQn(%i) is not a system interrupt", ipsr);
            break;
    }

    return lr;
}

void vmpu_acl_stack(uint8_t box_id, uint32_t bss_size, uint32_t stack_size, uint32_t * bss_start,
                    uint32_t * stack_pointer)
{
    static uint32_t g_box_mem_pos = 0;

    if (!g_box_mem_pos) {
        /* Initialize box memories. Leave stack-band sized gap. */
        g_box_mem_pos = UVISOR_REGION_ROUND_UP(
            (uint32_t)__uvisor_config.bss_boxes_start) +
            UVISOR_STACK_BAND_SIZE;
    }

    /* Ensure stack & context alignment. */
    stack_size = UVISOR_REGION_ROUND_UP(UVISOR_MIN_STACK(stack_size));

    /* Add stack ACL. */
    vmpu_region_add_static_acl(
        box_id,
        g_box_mem_pos,
        stack_size,
        UVISOR_TACLDEF_STACK,
        0
    );

    /* Set stack pointer to box stack size minus guard band. */
    g_box_mem_pos += stack_size;
    *stack_pointer = g_box_mem_pos;
    /* Add stack protection band. */
    g_box_mem_pos += UVISOR_STACK_BAND_SIZE;

    /* Add context ACL. */
    assert(bss_size != 0);
    bss_size = UVISOR_REGION_ROUND_UP(bss_size);
    *bss_start = g_box_mem_pos;

    DPRINTF("erasing box context at 0x%08X (%u bytes)\n",
        g_box_mem_pos,
        bss_size
    );

    /* Reset uninitialized secured box context. */
    memset(
        (void *) g_box_mem_pos,
        0,
        bss_size
    );

    /* Add context ACL. */
    vmpu_region_add_static_acl(
        box_id,
        g_box_mem_pos,
        bss_size,
        UVISOR_TACLDEF_DATA,
        0
    );

    g_box_mem_pos += bss_size + UVISOR_STACK_BAND_SIZE;
}

void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    /* Check for errors. */
    if (!vmpu_is_box_id_valid(src_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The source box ID is out of range (%u).\r\n", src_box);
    }
    if (!vmpu_is_box_id_valid(dst_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The destination box ID is out of range (%u).\r\n", dst_box);
    }

    /* Switch ACLs for peripherals. */
    vmpu_aips_switch(src_box, dst_box);

    /* Switch ACLs for memory regions. */
    vmpu_mem_switch(src_box, dst_box);
}

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size)
{
    /* Only support peripheral access and corner cases for now. */
    /* FIXME: Use SECURE_ACCESS for SCR! */
    if (fault_addr == (uint32_t) &SCB->SCR) {
        return UVISOR_TACL_UWRITE | UVISOR_TACL_UREAD;
    }

    /* Translate fault_addr into its physical address if it is in the bit-banding region. */
    if (VMPU_PERIPH_BITBAND_START <= fault_addr && fault_addr <= VMPU_PERIPH_BITBAND_END) {
        fault_addr = VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(fault_addr);
    } else if (VMPU_SRAM_BITBAND_START <= fault_addr && fault_addr <= VMPU_SRAM_BITBAND_END) {
        fault_addr = VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(fault_addr);
    }

    /* Look for ACL. */
    if ((AIPS0_BASE <= fault_addr) && (fault_addr < (AIPS0_BASE + 0xFEUL * AIPSx_SLOT_SIZE))) {
        return vmpu_fault_find_acl_aips(g_active_box, fault_addr, size);
    }

    return 0;
}

void vmpu_load_box(uint8_t box_id)
{
    if (box_id != 0) {
        HALT_ERROR(NOT_IMPLEMENTED, "currently only box 0 can be loaded");
    }
    vmpu_aips_switch(box_id, box_id);
    DPRINTF("%d  box %d loaded \n\r", box_id);
}

void vmpu_arch_init(void)
{
    vmpu_mpu_init();

    /* Init memory protection. */
    vmpu_mem_init();

    vmpu_mpu_lock();
}

void vmpu_order_boxes(int * const best_order, int box_count)
{
    for (int i = 0; i < box_count; ++i) {
        best_order[i] = i;
    }
}
