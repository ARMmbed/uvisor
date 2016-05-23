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
#include <uvisor.h>
#include <vmpu.h>
#include <svc.h>
#include <unvic.h>
#include <halt.h>
#include <debug.h>
#include <memory_map.h>
#include "vmpu_freescale_k64_aips.h"
#include "vmpu_freescale_k64_mem.h"

uint32_t g_box_mem_pos;

/* TODO/FIXME: implement recovery from Freescale MPU fault */
static int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp)
{
    /* FIXME: currently we deny every access */
    /* TODO: implement actual recovery */
    return -1;
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
            DEBUG_FAULT(FAULT_MEMMANAGE, lr, lr & 0x4 ? psp : msp);
            break;

        case BusFault_IRQn:
            /* where a Freescale MPU is used, bus faults can originate both as
             * pure bus faults or as MPU faults; in addition, they can be both
             * precise and imprecise
             * there is also an additional corner case: some registers (MK64F)
             * cannot be accessed in unprivileged mode even if an MPU region is
             * created for them and the corresponding bit in PACRx is set. In
             * some cases we want to allow access for them (with ACLs), hence we
             * use a function that looks for a specially crafted opcode */

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

                /* check if the fault is an MPU fault */
                if (MPU->CESR >> 27 && !vmpu_fault_recovery_mpu(pc, psp)) {
                    VMPU_SCB_BFSR = fault_status;
                    return;
                }

                /* check if the fault is the special register corner case */
                if (!vmpu_fault_recovery_bus(pc, psp, fault_addr, fault_status)) {
                    VMPU_SCB_BFSR = fault_status;
                    return;
                }

                /* if recovery was not successful, throw an error and halt */
                DEBUG_FAULT(FAULT_BUS, lr, psp);
                VMPU_SCB_BFSR = fault_status;
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
            break;

        case HardFault_IRQn:
            DEBUG_FAULT(FAULT_HARD, lr, lr & 0x4 ? psp : msp);
            break;

        case DebugMonitor_IRQn:
            DEBUG_FAULT(FAULT_DEBUG, lr, lr & 0x4 ? psp : msp);
            break;

        default:
            HALT_ERROR(NOT_ALLOWED, "Active IRQn(%i) is not a system interrupt", ipsr);
            break;
    }
}

void vmpu_acl_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    int res;

#ifndef NDEBUG
    const MemMap *map;
#endif/*NDEBUG*/

    /* check for maximum box ID */
    if (!vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL add: The box ID is out of range (%u).\r\n", box_id);
    }

    /* check for alignment to 32 bytes */
    if(((uint32_t)start) & 0x1F)
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL start address is not aligned [0x%08X]\n", start);

    /* round ACLs if needed */
    if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
        size = UVISOR_REGION_ROUND_DOWN(size);
    else
        if(acl & UVISOR_TACL_SIZE_ROUND_UP)
            size = UVISOR_REGION_ROUND_UP(size);

    DPRINTF("\t@0x%08X size=%06i acl=0x%04X [%s]\n", start, size, acl,
        ((map = memory_map_name((uint32_t)start))!=NULL) ? map->name : "unknown"
    );

    /* check for peripheral memory, proceed with general memory */
    if(acl & UVISOR_TACL_PERIPHERAL)
        res = vmpu_aips_add(box_id, start, size, acl);
    else
        res = vmpu_mem_add(box_id, start, size, acl);

    if(!res)
        HALT_ERROR(NOT_ALLOWED, "ACL in unhandled memory area\n");
    else
        if(res<0)
            HALT_ERROR(SANITY_CHECK_FAILED, "ACL sanity check failed [%i]\n", res);
}

void vmpu_acl_stack(uint8_t box_id, uint32_t bss_size, uint32_t stack_size)
{
    /* handle main box */
    if(!box_id)
    {
        DPRINTF("ctx=%i stack=%i\n\r", bss_size, stack_size);
        /* non-important sanity checks */
        assert(bss_size == 0);
        assert(stack_size == 0);

        /* assign main box stack pointer to existing
         * unprivileged stack pointer */
        g_svc_cx_curr_sp[0] = (uint32_t*)__get_PSP();
        g_svc_cx_context_ptr[0] = NULL;
        return;
    }

    /* ensure stack & context alignment */
    stack_size = UVISOR_REGION_ROUND_UP(UVISOR_MIN_STACK(stack_size));

    /* add stack ACL */
    vmpu_acl_add(
        box_id,
        (void*)g_box_mem_pos,
        stack_size,
        UVISOR_TACLDEF_STACK
    );

    /* set stack pointer to box stack size minus guard band */
    g_box_mem_pos += stack_size;
    g_svc_cx_curr_sp[box_id] = (uint32_t*)g_box_mem_pos;
    /* add stack protection band */
    g_box_mem_pos += UVISOR_STACK_BAND_SIZE;

    /* add context ACL if needed */
    if(!bss_size)
        g_svc_cx_context_ptr[box_id] = NULL;
    else
    {
        bss_size = UVISOR_REGION_ROUND_UP(bss_size);
        g_svc_cx_context_ptr[box_id] = (uint32_t*)g_box_mem_pos;

        DPRINTF("erasing box context at 0x%08X (%u bytes)\n",
            g_box_mem_pos,
            bss_size
        );

        /* reset uninitialized secured box context */
        memset(
            (void *) g_box_mem_pos,
            0,
            bss_size
        );

        /* add context ACL */
        vmpu_acl_add(
            box_id,
            (void*)g_box_mem_pos,
            bss_size,
            UVISOR_TACLDEF_DATA
        );

        g_box_mem_pos += bss_size + UVISOR_STACK_BAND_SIZE;
    }
}

void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    /* check for errors */
    if (!vmpu_is_box_id_valid(dst_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The destination box ID is out of range (%u).\r\n", dst_box);
    }

    /* switch ACLs for peripherals */
    vmpu_aips_switch(src_box, dst_box);

    /* switch ACLs for memory regions */
    vmpu_mem_switch(src_box, dst_box);
}

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size)
{
    /* only support peripheral access and corner cases for now */
    switch (fault_addr) {
        case (uint32_t) &SCB->SCR:
            return UVISOR_TACL_UWRITE | UVISOR_TACL_UREAD;
        default:
            /* translate fault_addr into its physical address if it is in the bit-banding region */
            if (fault_addr >= VMPU_PERIPH_BITBAND_START && fault_addr <= VMPU_PERIPH_BITBAND_END) {
                fault_addr = VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(fault_addr);
            }
            else if (fault_addr >= VMPU_SRAM_BITBAND_START && fault_addr <= VMPU_SRAM_BITBAND_END) {
                fault_addr = VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(fault_addr);
            }

            /* look for ACL */
            if( (fault_addr>=AIPS0_BASE) &&
                ((fault_addr<(AIPS0_BASE+(0xFEUL*(AIPSx_SLOT_SIZE))))) )
                return vmpu_fault_find_acl_aips(g_active_box, fault_addr, size);
            else
                return vmpu_fault_find_acl_mem(g_active_box, fault_addr, size);
    }
}


void vmpu_load_box(uint8_t box_id)
{
    if(box_id != 0)
    {
        HALT_ERROR(NOT_IMPLEMENTED, "currently only box 0 can be loaded");
    }
    vmpu_aips_switch(box_id, box_id);
    DPRINTF("%d  box %d loaded \n\r", box_id);
}

void vmpu_arch_init(void)
{
    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

    /* initialize box memories, leave stack-band sized gap */
    g_box_mem_pos = UVISOR_REGION_ROUND_UP(
        (uint32_t)__uvisor_config.bss_boxes_start) +
        UVISOR_STACK_BAND_SIZE;

    /* init memory protection */
    vmpu_mem_init();
}
