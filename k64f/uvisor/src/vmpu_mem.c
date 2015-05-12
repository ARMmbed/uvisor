/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/

#include <uvisor.h>
#include "svc.h"
#include "vmpu.h"
#include "vmpu_mem.h"



static int vmpu_add_mem_int(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    return 1;
}

int vmpu_add_mem(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    if(    (((uint32_t*)start)>=__uvisor_config.secure_start) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.secure_end) )
    {
        DPRINTF("FLASH\n");

        return vmpu_add_mem_int(box_id, start, size, acl & UVISOR_TACLDEF_SECURE_CONST);
    }

    if(    (((uint32_t*)start)>=__uvisor_config.reserved_end) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.bss_start) )
    {
        DPRINTF("STACK\n");

        return vmpu_add_mem_int(box_id, start, size, acl & UVISOR_TACLDEF_STACK);
    }

    if(    (((uint32_t*)start)>=__uvisor_config.bss_start) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.bss_end) )
    {
        DPRINTF("BSS\n");

        return vmpu_add_mem_int(box_id, start, size, acl & UVISOR_TACLDEF_SECURE_BSS);
    }

    return 0;
}
