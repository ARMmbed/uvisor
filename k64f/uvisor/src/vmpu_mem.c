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

#define MPU_MAX_REGIONS 12

typedef struct
{
    uint32_t word[3];
    uint32_t acl;
} TMemACL;

typedef struct
{
    TMemACL *acl;
    uint32_t count;
} TBoxACL;

uint32_t g_mem_acl_count, g_mem_acl_user;
static TMemACL g_mem_acl[UVISOR_MAX_ACLS];
static TBoxACL g_mem_box[UVISOR_MAX_BOXES];

int vmpu_mem_switch(uint8_t src_box, uint8_t dst_box)
{
    uint32_t t,u;
    TBoxACL *box;
    TMemACL *rgd;

    /* get box config */
    if((dst_box >= UVISOR_MAX_BOXES)||(src_box >= UVISOR_MAX_BOXES))
        return -1;
    box = &g_mem_box[dst_box];

    /* disable previous ACL */
    if(src_box!=dst_box)
    {
        if((u = g_mem_box[src_box].count)==0)
            return -2;

        u += g_mem_acl_user;
        if(u>MPU_MAX_REGIONS)
            return -3;

        t=g_mem_acl_count;
        while(t<u)
        {
            MPU->WORD[t][3] = 0;
            t++;
        }
    }

    /* get mem ACL */
    rgd = &box->acl[box->count];
    /* already populated - ensure to fill boxes unfragmented */
    if(!rgd)
        return -4;

    /* copy and enable MPU region */
    for(t=g_mem_acl_user; t<g_mem_acl_count; t++)
    {
        memcpy(MPU->WORD, rgd->word, sizeof(rgd->word));
        rgd->word[3] = 1;
    }

    DPRINTF("DONE\n");
    while(1);

    return -1;
}

static int vmpu_mem_overlap(uint32_t s1, uint32_t e1, uint32_t s2, uint32_t e2)
{
    return (
        (s1>e1)                ||
        (s2>e2)                || /* punish messing with overlap */
        ((s1<=s2) && (e1>=s2)) ||
        ((s1<=e2) && (e1>=e2)) ||
        ((s1>=e2) && (e1<=e2))
        );
}

static int vmpu_mem_add_int(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    uint32_t perm, end, t;
    TBoxACL *box;
    TMemACL *rgd;

    /* handle empty or fully protected regions */
    if(!size || !(acl & (UVISOR_TACL_UACL|UVISOR_TACL_SACL)))
        return 1;

    /* ensure that ACL size can be rounded up to slot size */
    if(size % 32)
    {
        if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
            size = UVISOR_ROUND32_DOWN(size);
        else
            if(acl & UVISOR_TACL_SIZE_ROUND_UP)
                size = UVISOR_ROUND32_UP(size);
            else
                {
                    DPRINTF("Use UVISOR_TACL_SIZE_ROUND_UP/*_DOWN to round ACL size");
                    return -21;
                }
    }

    /* ensure that ACL base is a multiple of 32 */
    if((((uint32_t)start) % 32) != 0)
    {
        DPRINTF("start address needs to be aligned on a 32 bytes border");
        return -22;
    }

    /* get box config */
    if(box_id >= UVISOR_MAX_BOXES)
        return -23;
    box = &g_mem_box[box_id];

    /* initialize acl pointer */
    if(!box->acl)
    {
        /* check for integer overflow */
        if( g_mem_acl_count >= UVISOR_MAX_ACLS )
            return -24;
        box->acl = &g_mem_acl[g_mem_acl_count];
    }

    /* check for precise overflow */
    if( (&box->acl[box->count] - g_mem_acl)>=(UVISOR_MAX_ACLS-1) )
        return -25;

    /* check if region overlaps */
    end = ((uint32_t) start) + size;
    rgd = g_mem_acl;
    for(t=0; t<g_mem_acl_count; t++, rgd++)
        if(vmpu_mem_overlap((uint32_t) start, end, rgd->word[0], rgd->word[1]))
        {
            DPRINTF("Detected overlap with ACL %i (0x%08X-0x%08X)",
                t, rgd->word[0], rgd->word[1]);

            /* handle permitted overlaps */
            if(!((rgd->acl & UVISOR_TACL_SHARED) &&
                (acl & UVISOR_TACL_SHARED)))
                return -26;
        }

    /* get mem ACL */
    rgd = &box->acl[box->count];
    /* already populated - ensure to fill boxes unfragmented */
    if(rgd->acl)
        return -27;

    /* remember original ACL */
    rgd->acl = acl;
    /* start address, aligned tro 32 bytes */
    rgd->word[0] = (uint32_t) start;
    /* end address, aligned tro 32 bytes */
    rgd->word[1] = end;

    /* handle user permissions */
    perm = (acl & UVISOR_TACL_USER) ?  acl & UVISOR_TACL_UACL : 0;

    /* if S-perms are identical to U-perms, refer from S to U */
    if(((acl & UVISOR_TACL_SACL)>>3) == perm)
        perm |= 3UL << 3;
    else
        /* handle detailed supervisor permissions */
        switch(acl & UVISOR_TACL_SACL)
        {
            case UVISOR_TACL_SREAD|UVISOR_TACL_SWRITE|UVISOR_TACL_SEXECUTE:
                perm |= 0UL << 3;
                break;

            case UVISOR_TACL_SREAD|UVISOR_TACL_SEXECUTE:
                perm |= 1UL << 3;
                break;

            case UVISOR_TACL_SREAD|UVISOR_TACL_SWRITE:
                perm |= 2UL << 3;
                break;

            default:
                DPRINTF("chosen supervisor ACL's are not supported by hardware (0x%08X)\n", acl);
                return -7;
        }
    rgd->word[2] = perm;

    /* increment ACL count */
    box->count++;
    /* increment total ACL count */
    g_mem_acl_count++;

    return 1;
}

int vmpu_mem_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    if(    (((uint32_t*)start)>=__uvisor_config.secure_start) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.secure_end) )
    {
        DPRINTF("\t\tSECURE_FLASH\n");

        return vmpu_mem_add_int(box_id, start, size, acl & UVISOR_TACLDEF_SECURE_CONST);
    }

    if(    (((uint32_t*)start)>=__uvisor_config.reserved_end) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.bss_start) )
    {
        DPRINTF("\t\tSECURE_STACK\n");

        /* disallow user to provide stack region ACL's */
        if(acl & UVISOR_TACL_USER)
            return -1;

        return vmpu_mem_add_int(box_id, start, size, (acl & UVISOR_TACLDEF_STACK)|UVISOR_TACL_STACK);
    }

    if(    (((uint32_t*)start)>=__uvisor_config.bss_start) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.bss_end) )
    {
        DPRINTF("\t\tSECURE_BSS\n");

        return vmpu_mem_add_int(box_id, start, size, acl & UVISOR_TACLDEF_SECURE_BSS);
    }

    return 0;
}

void vmpu_mem_init(void)
{
    /* uvisor SRAM only accessible to uvisor */
    vmpu_mem_add_int(0, (void*)SRAM_ORIGIN, UVISOR_SRAM_SIZE,
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SWRITE);

    /* rest of SRAM, accessible to mbed - non-executable for uvisor */
    vmpu_mem_add_int(0, (void*)(SRAM_ORIGIN+UVISOR_SRAM_SIZE), SRAM_LENGTH-UVISOR_SRAM_SIZE,
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SWRITE|
        UVISOR_TACL_UREAD|
        UVISOR_TACL_UWRITE|
        UVISOR_TACL_UEXECUTE|
        UVISOR_TACL_USER
        );

    /* enable read access to unsecure flash regions - allow execution */
    vmpu_mem_add_int(0, (void*)FLASH_ORIGIN, ((uint32_t)__uvisor_config.secure_start)-FLASH_ORIGIN,
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SEXECUTE|
        UVISOR_TACL_UREAD|
        UVISOR_TACL_UEXECUTE|
        UVISOR_TACL_USER);

    /* uvisor SRAM only accessible to uvisor */
    vmpu_mem_add_int(0, (void*)SRAM_ORIGIN, UVISOR_SRAM_SIZE,
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SWRITE);

    /* initial primary box ACL's */
    vmpu_mem_switch(0, 0);

    /* mark all primary box ACL's as static */
    g_mem_acl_user = g_mem_acl_count;
}
