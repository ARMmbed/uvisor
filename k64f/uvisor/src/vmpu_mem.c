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

typedef struct
{
    uint32_t start, size, acl;
    uint32_t usage;
} TMemACL;

typedef struct
{
    TMemACL *acl;
    uint32_t count;
} TBoxACL;

uint32_t g_mem_acl_count;
static TMemACL g_mem_acl[UVISOR_MAX_ACLS];
static TBoxACL g_mem_box[UVISOR_MAX_BOXES];

static int vmpu_add_mem_int(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    TBoxACL *box;
    TMemACL *mem;

    assert(g_mem_acl_count < UVISOR_MAX_ACLS);
    box = &g_mem_box[box_id];

    /* handle empty regions */
    if(!size)
        return 1;

    /* check for integer overflow */
    if( g_mem_acl_count >= UVISOR_MAX_ACLS )
        return -1;

    /* initialize acl pointer */
    if(!box->acl)
        box->acl = &g_mem_acl[g_mem_acl_count];

    /* check for precise overflow */
    if( (&box->acl[box->count] - g_mem_acl)>=(UVISOR_MAX_ACLS-1) )
        return -2;

    /* get mem ACL */
    mem = &box->acl[box->count];
    /* already populated - ensure to fill boxes unfragmented */
    if(mem->size)
        return -3;

    /* increment ACL count */
    box->count++;
    g_mem_acl_count++;

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
