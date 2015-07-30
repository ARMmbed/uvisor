/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include <vmpu.h>
#include <svc.h>
#include <halt.h>
#include <debug.h>
#include <memory_map.h>
#include "vmpu_freescale_k64_aips.h"
#include "vmpu_freescale_k64_mem.h"

uint32_t g_box_mem_pos;

void MemoryManagement_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_MEMMANAGE);
    halt_led(FAULT_MEMMANAGE);
}

/* FIXME check if fault came from ldr/str instruction, check ACL and, if
 * allowed, perform operation and return to regular execution */
void BusFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_BUS);

    /* the Freescale MPU results in bus faults when an access is forbidden;
     * hence, a different error pattern is used depending on the case */
    /* Note: since we are halting execution we don't bother clearing the SPERR
     *       bit in the MPU->CESR register */
    if(MPU->CESR >> 27)
        halt_led(NOT_ALLOWED);
    else
        halt_led(FAULT_BUS);
}

void UsageFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_USAGE);
    halt_led(FAULT_USAGE);
}

void HardFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_HARD);
    halt_led(FAULT_HARD);
}

void DebugMonitor_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_DEBUG);
    halt_led(FAULT_DEBUG);
}

void vmpu_acl_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    int res;

#ifndef NDEBUG
    const MemMap *map;
#endif/*NDEBUG*/

    /* check for maximum box ID */
    if(box_id>=UVISOR_MAX_BOXES)
        HALT_ERROR(SANITY_CHECK_FAILED, "box ID out of range (%i)\n", box_id);

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

void vmpu_acl_stack(uint8_t box_id, uint32_t context_size, uint32_t stack_size)
{
    /* handle main box */
    if(!box_id)
    {
        DPRINTF("ctx=%i stack=%i\n\r", context_size, stack_size);
        /* non-important sanity checks */
        assert(context_size == 0);
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
    if(!context_size)
        g_svc_cx_context_ptr[box_id] = NULL;
    else
    {
        context_size = UVISOR_REGION_ROUND_UP(context_size);
        g_svc_cx_context_ptr[box_id] = (uint32_t*)g_box_mem_pos;

        /* add context ACL */
        vmpu_acl_add(
            box_id,
            (void*)g_box_mem_pos,
            context_size,
            UVISOR_TACLDEF_DATA
        );

        g_box_mem_pos += context_size + UVISOR_STACK_BAND_SIZE;
    }
}

int vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    /* switch ACLs for peripherals */
    vmpu_aips_switch(src_box, dst_box);

    /* switch ACLs for memory regions */
    vmpu_mem_switch(src_box, dst_box);

    return 0;
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
        (uint32_t)__uvisor_config.reserved_end) +
        UVISOR_STACK_BAND_SIZE;

    /* init memory protection */
    vmpu_mem_init();
}
