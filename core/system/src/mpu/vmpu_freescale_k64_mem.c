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
#include "halt.h"
#include "vmpu.h"
#include "vmpu_freescale_k64_mem.h"
#include "page_allocator_faults.h"

/* The K64F has 12 MPU regions, however, we use region 0 as the background
 * region with `UVISOR_TACL_BACKGROUND` as permissions.
 * Region 1 and 2 are used to unlock Application SRAM and Flash.
 * Therefore 9 MPU regions are available for user ACLs.
 * Region 3 and 4 are used to protect the current box stack and context.
 * This leaves 6 MPU regions for round robin scheduling:
 *
 *     12      <-- End of MPU regions, K64F_MPU_REGIONS_MAX
 * +---------+
 * |   11    |
 * |   ...   |
 * |    5    | <-- Box Pages, K64F_MPU_REGIONS_USER
 * +---------+
 * |    4    | <-- Box Context
 * |    3    | <-- Box Stack, K64F_MPU_REGIONS_STATIC
 * +---------+
 * |    2    | <-- Application Flash unlock
 * |    1    | <-- Application SRAM unlock
 * |    0    | <-- Background region
 * +---------+
 */
#define K64F_MPU_REGIONS_STATIC 3
#define K64F_MPU_REGIONS_USER 5
#define K64F_MPU_REGIONS_MAX 12

typedef struct
{
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t permissions;
    uint32_t acl;
} TMemACL;

typedef struct
{
    TMemACL *acl;
    uint32_t count;
} TBoxACL;

uint32_t g_mem_acl_count, g_mpu_slot;
static TMemACL g_mem_acl[UVISOR_MAX_ACLS];
static TBoxACL g_mem_box[UVISOR_MAX_BOXES];

int vmpu_mem_push_page_acl(uint32_t start_addr, uint32_t end_addr)
{
    MPU_Region * region;

    /* Check that start and end address are aligned to 32-byte. */
    if (start_addr & 0x1F || end_addr & 0x1F) {
        return -1;
    }

    /* Insert into MPU regions in RR fashion. */
    region = (MPU_Region *) MPU->WORD[g_mpu_slot];
    region->STARTADDR = start_addr;
    region->ENDADDR = end_addr;
    /* Permissions are fixed to only allow unprivileged write/read/execute. Duh. */
    region->PERMISSIONS = 0x1E;
    region->CONTROL = 1;

    /* DPRINTF("Inserting page region [0x%08x, 0x%08x] at MPU slot %u\n", start_addr, end_addr, g_mpu_slot); */
    g_mpu_slot++;

    /* Handle slot index overflow. */
    if (g_mpu_slot >= K64F_MPU_REGIONS_MAX) {
        g_mpu_slot = K64F_MPU_REGIONS_USER;
    }
    return 0;
}

/* FIXME - test for actual ACLs */
uint32_t vmpu_fault_find_acl_mem(uint8_t box_id, uint32_t fault_addr, uint32_t size)
{
    return 0;
}

/* This is the iterator callback for inserting all page heap ACLs into to the
 * MPU during `vmpu_mem_switch()`. */
static int vmpu_mem_push_page_acl_iterator(uint32_t start_addr, uint32_t end_addr, uint8_t page)
{
    /* Insert the MPU region at the `g_mpu_slot`. */
    vmpu_mem_push_page_acl(start_addr, end_addr);
    /* We only continue if we have not wrapped around the end of the MPU regions yet. */
    return (g_mpu_slot > K64F_MPU_REGIONS_USER);
}

void vmpu_mem_switch(uint8_t src_box, uint8_t dst_box)
{
    uint32_t t, u, dst_count;
    MPU_Region * region;
    TBoxACL *box;
    TMemACL *rgd;

    /* Sanity checks. */
    if (!vmpu_is_box_id_valid(src_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The source box ID is out of range (%u).\r\n", src_box);
    }
    if (!vmpu_is_box_id_valid(dst_box)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The destination box ID is out of range (%u).\n", dst_box);
    }

    box = &g_mem_box[dst_box];

    /* For box zero, only copy the page heap ACLs, disable the remaining pages
     * and then return. */
    if(src_box && !dst_box) {
        g_mpu_slot = K64F_MPU_REGIONS_STATIC;
        t = K64F_MPU_REGIONS_MAX - K64F_MPU_REGIONS_STATIC;
        if (page_allocator_iterate_active_pages(vmpu_mem_push_page_acl_iterator) < t) {
            /* Disable all remaining regions. */
            for (t = g_mpu_slot; t < K64F_MPU_REGIONS_MAX; t++) {
                ((MPU_Region *) MPU->WORD[t])->CONTROL = 0;
            }
        }
        return;
    }

    dst_count = g_mem_box[dst_box].count;
    assert(dst_count);

    /* already populated - ensure to fill boxes unfragmented */
    rgd = box->acl;
    assert(rgd);

    /* Update MPU regions:
     * We assume that the first two ACLs belonging to the box are the stack
     * and context ACLs. Therefore we start at index K64F_MPU_REGIONS_STATIC.
     * We opportunistically copy all remaining ACLs into the MPU regions, until
     * the regions run out. The remaining ACLs are switched in on demand. */
    u = K64F_MPU_REGIONS_STATIC + dst_count;
    /* The round robin _starts_ at the first free MPU region _after_ the box
     * ACLs have been copied. */
    g_mpu_slot = u;
    if (u >= K64F_MPU_REGIONS_MAX) {
        /* Clamp u to K64F_MPU_REGIONS_MAX. */
        u = K64F_MPU_REGIONS_MAX;
        /* If no MPU regions are free anymore, set the RR marker at the
         * of the USER MPU regions (leaving stack and context intact!). */
        g_mpu_slot = K64F_MPU_REGIONS_USER;
    } else {
        /* Copy the active page heap ACLs into the MPU regions:
         * This will start at index `g_mpu_slot` and continue to `K64F_MPU_REGIONS_MAX`. */
        /* FIXME: Use page fault count to prioritize which pages are enabled! */
        t = K64F_MPU_REGIONS_MAX - g_mpu_slot;
        if (page_allocator_iterate_active_pages(vmpu_mem_push_page_acl_iterator) < t) {
            /* Disable all remaining regions. */
            for (t = g_mpu_slot; t < K64F_MPU_REGIONS_MAX; t++) {
                ((MPU_Region *) MPU->WORD[t])->CONTROL = 0;
            }
        }
    }
    /* Copy the ACLs into the MPU regions. */
    for (t = K64F_MPU_REGIONS_STATIC; t < u; t++, rgd++) {
        region = (MPU_Region *) MPU->WORD[t];
        region->STARTADDR = rgd->start_addr;
        region->ENDADDR = rgd->end_addr;
        region->PERMISSIONS = rgd->permissions;
        region->CONTROL = 1;
    }
}

static int vmpu_mem_overlap(uint32_t s1, uint32_t e1, uint32_t s2, uint32_t e2)
{
    return (
        (s1>e1)                ||
        (s2>e2)                || /* punish messing with overlap */
        ((s1<=s2) && (e1> s2)) ||
        ((s1< e2) && (e1>=e2)) ||
        ((s1>=s2) && (e1<=e2))
        );
}

static int vmpu_mem_add_int(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    uint32_t perm, end, t;
    TBoxACL *box;
    TMemACL *rgd;

    DPRINTF(" mem_acl[%i]: 0x%08X-0x%08X (size=%i, acl=0x%04X)\n",
            g_mem_acl_count, start, ((uint32_t)start)+size, size, acl);

    /* handle empty or fully protected regions */
    if(!size || !(acl & (UVISOR_TACL_UACL|UVISOR_TACL_SACL)))
        return 1;

    /* ensure that ACL size can be rounded up to slot size */
    if(size % 32)
    {
        if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
            size = UVISOR_REGION_ROUND_DOWN(size);
        else
            if(acl & UVISOR_TACL_SIZE_ROUND_UP)
                size = UVISOR_REGION_ROUND_UP(size);
            else
                {
                    DPRINTF("use UVISOR_TACL_SIZE_ROUND_UP/*_DOWN to round ACL size\n");
                    return -21;
                }
    }

    /* ensure that ACL base is a multiple of 32 */
    if((((uint32_t)start) % 32) != 0)
    {
        DPRINTF("start address needs to be aligned on a 32 bytes border\n");
        return -22;
    }

    /* get box config */
    if (!vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vMPU switch: The box ID is out of range (%u).\n", box_id);
    }
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
        if(vmpu_mem_overlap((uint32_t) start, end, rgd->start_addr, rgd->end_addr))
        {
            DPRINTF("detected overlap with ACL %i (0x%08X-0x%08X)\n",
                t, rgd->start_addr, rgd->end_addr);

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
    rgd->start_addr = (uint32_t) start;
    /* end address, aligned tro 32 bytes */
    rgd->end_addr = end;

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
    rgd->permissions = perm;

    /* increment ACL count */
    box->count++;
    /* increment total ACL count */
    g_mem_acl_count++;

    return 1;
}

int vmpu_mem_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    if(    (((uint32_t*)start)>=__uvisor_config.bss_boxes_start) &&
        ((((uint32_t)start)+size)<=(uint32_t)__uvisor_config.bss_boxes_end) )
    {
        DPRINTF("\t\tSECURE_STACK\n");

        /* disallow user to provide stack region ACL's */
        if(acl & UVISOR_TACL_USER)
            return -1;

        return vmpu_mem_add_int(box_id, start, size,
                                (acl & UVISOR_TACLDEF_STACK) | UVISOR_TACL_STACK | UVISOR_TACL_USER);
    }

    return 0;
}

void vmpu_mem_init(void)
{
    int res;
    uint32_t t;
    TMemACL * rgd;
    MPU_Region * region;

    /* rest of SRAM, accessible to mbed - non-executable for uvisor */
    res = vmpu_mem_add_int(0, __uvisor_config.bss_end,
        ((uint32_t) __uvisor_config.sram_end) - ((uint32_t) __uvisor_config.bss_end),
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SWRITE|
        UVISOR_TACL_UREAD|
        UVISOR_TACL_UWRITE|
        UVISOR_TACL_UEXECUTE|
        UVISOR_TACL_USER
        );
    if( res<0 )
        HALT_ERROR(SANITY_CHECK_FAILED, "failed setting up box 0 SRAM (%i)\n", res);

    /* enable read access to unsecure flash regions - allow execution */
    res = vmpu_mem_add_int(0, (void*)FLASH_ORIGIN, ((uint32_t)__uvisor_config.secure_end)-FLASH_ORIGIN,
        UVISOR_TACL_SREAD|
        UVISOR_TACL_SEXECUTE|
        UVISOR_TACL_UREAD|
        UVISOR_TACL_UEXECUTE|
        UVISOR_TACL_USER);
    if( res<0 )
        HALT_ERROR(SANITY_CHECK_FAILED, "failed setting up box FLASH (%i)\n", res);

    /* initial primary box ACL's */
    rgd = g_mem_box[0].acl;
    /* Copy ACLs [0, 1] into MPU regions [_1_, _2_]. Region 0 is the background region! */
    for(t = 1; t < K64F_MPU_REGIONS_STATIC; t++, rgd++) {
        region = (MPU_Region *) MPU->WORD[t];
        region->STARTADDR = rgd->start_addr;
        region->ENDADDR = rgd->end_addr;
        region->PERMISSIONS = rgd->permissions;
        region->CONTROL = 1;
    }

    /* MPU background region permission mask
     *   this mask must be set as last one, since the background region gives no
     *   executable privileges to neither user nor supervisor modes */
    MPU->RGDAAC[0] = UVISOR_TACL_BACKGROUND;
}
