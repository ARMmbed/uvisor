/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#include "vmpu.h"
#include "vmpu_mpu.h"
#include "vmpu_freescale_k64_aips.h"
#include "vmpu_freescale_k64_mem.h"

/* This file contains the configuration-specific symbols. */
#include "configurations.h"

typedef struct
{
    MpuRegion * regions;
    uint32_t count;
} MpuRegionSlice;

static uint16_t g_mpu_region_count;
static MpuRegion g_mpu_region[UVISOR_MAX_ACLS];
static MpuRegionSlice g_mpu_box_region[UVISOR_MAX_BOXES];

int vmpu_is_region_size_valid(uint32_t size)
{
    /* Align size to 32B. */
    const uint32_t masked_size = size & ~31;
    if (masked_size < 32 || (1 << 29) < masked_size) {
        /* 2^5 == 32, which is the minimum region size. */
        /* 2^29 == 512M, which is the maximum region size. */
        return 0;
    }
    /* There is no rounding, we only care about an exact match! */
    return (masked_size == size);
}

uint32_t vmpu_round_up_region(uint32_t addr, uint32_t size)
{
    if (!vmpu_is_region_size_valid(size)) {
        /* Region size must be valid! */
        return 0;
    }
    /* Alignment is always 32B. */
    const uint32_t mask = 31;

    /* Adding the mask can overflow. */
    const uint32_t rounded_addr = addr + mask;
    /* Check for overflow. */
    if (rounded_addr < addr) {
        /* This means the address was too large to align. */
        return 0;
    }
    /* Mask the rounded address to get the aligned address. */
    return (rounded_addr & ~mask);
}

static uint32_t vmpu_map_acl(UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    /* handle user permissions */
    uint32_t perm = (acl & UVISOR_TACL_USER) ? (acl & UVISOR_TACL_UACL) : 0;

    /* Allow uVisor-internal code to extend permissions for certain
     * ACLs by hardware-specific ACLs like DMA busmaster access.
     * Ensure that core ACLs are specified trough the generic ACL
     * method by masking it out. */
    perm |= (acl_hw_spec & ~0x3FUL);

    /* if S-perms are identical to U-perms, refer from S to U */
    if(((acl & UVISOR_TACL_SACL) >> 3) == perm) {
        perm |= 3UL << 3;
    }
    else {
        /* handle detailed supervisor permissions */
        switch(acl & UVISOR_TACL_SACL)
        {
            case UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE | UVISOR_TACL_SEXECUTE:
                perm |= 0UL << 3;
                break;

            case UVISOR_TACL_SREAD | UVISOR_TACL_SEXECUTE:
                perm |= 1UL << 3;
                break;

            case UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE:
                perm |= 2UL << 3;
                break;

            default:
                DPRINTF("chosen supervisor ACL's are not supported by hardware (0x%08X)\n", acl);
                return 0;
        }
    }
    return perm;
}

uint32_t vmpu_region_translate_acl(MpuRegion * const region, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    /* ensure that ACL size can be rounded up to slot size */
    if(size % 32)
    {
        if(acl & UVISOR_TACL_SIZE_ROUND_DOWN) {
            size = UVISOR_REGION_ROUND_DOWN(size);
        }
        else if(acl & UVISOR_TACL_SIZE_ROUND_UP) {
            size = UVISOR_REGION_ROUND_UP(size);
        }
        else {
            DPRINTF("use UVISOR_TACL_SIZE_ROUND_UP/*_DOWN to round ACL size\n");
            return 0;
        }
    }

    /* ensure that ACL base is a multiple of 32 */
    if(start % 32) {
        DPRINTF("start address needs to be aligned on a 32 bytes border\n");
        return 0;
    }

    region->config = vmpu_map_acl(acl, acl_hw_spec);
    region->start = start;
    region->end = start + size;
    region->acl = acl;

    return size;

}



static int vmpu_mem_overlap(uint32_t s1, uint32_t e1, uint32_t s2, uint32_t e2)
{
    return (
        (s1 > e1)                  ||
        (s2 > e2)                  || /* punish messing with overlap */
        ((s1 <= s2) && (e1 >  s2)) ||
        ((s1 <  e2) && (e1 >= e2)) ||
        ((s1 >= s2) && (e1 <= e2)) );
}

static uint32_t vmpu_region_add_static_acl_internal(uint8_t box_id, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion * region;
    MpuRegionSlice * box;
    uint32_t rounded_size, end, t;

    DPRINTF(" mem_acl[%i]: 0x%08X-0x%08X (size=%i, acl=0x%04X)\n",
            g_mpu_region_count, start, ((uint32_t)start)+size, size, acl);

    /* handle empty or fully protected regions */
    if(!size || !(acl & (UVISOR_TACL_UACL|UVISOR_TACL_SACL)))
        return 1;

    if(g_mpu_region_count >= UVISOR_MAX_ACLS) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_add_static_acl: No more regions to allocate!\n");
    }

    if (!vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_add_static_acl: The box id (%u) is out of range.\n", box_id);
    }

    /* check if region overlaps */
    end = start + size;
    region = g_mpu_region;

    for (t=0; t < g_mpu_region_count; t++, region++)
    {
        if (vmpu_mem_overlap(start, end, region->start, region->end)) {
            DPRINTF("detected overlap with ACL %i (0x%08X-0x%08X)\n",
                t, region->start, region->end);

            /* handle permitted overlaps */
            if(!((region->acl & UVISOR_TACL_SHARED) && (acl & UVISOR_TACL_SHARED))) {
                return 0;
            }
        }
    }

    box = &g_mpu_box_region[box_id];
    if (!box->regions) {
        box->regions = &g_mpu_region[g_mpu_region_count];
    }

    region = &box->regions[box->count];
    if (region->config) {
        HALT_ERROR(SANITY_CHECK_FAILED, "unordered region allocation\n");
    }

    rounded_size = vmpu_region_translate_acl(region, start, size,
        acl, acl_hw_spec);

    box->count++;
    g_mpu_region_count++;

    return rounded_size;
}

uint32_t vmpu_region_add_static_acl(uint8_t box_id, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
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
    if(start & 0x1F) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL start address is not aligned [0x%08X]\n", start);
    }

    DPRINTF("\t@0x%08X size=%06i acl=0x%04X [%s]\n", start, size, acl,
        (map = memory_map_name(start)) ? map->name : "unknown");

    /* check for peripheral memory, proceed with general memory */
    if(acl & UVISOR_TACL_PERIPHERAL) {
        res = vmpu_aips_add(box_id, (void *) start, size, acl);
    }
    else {
        size = vmpu_region_add_static_acl_internal(box_id, start, size,
            (acl & UVISOR_TACLDEF_STACK) | UVISOR_TACL_STACK | UVISOR_TACL_USER,
            acl_hw_spec);
        res = size ? 1 : 0;
    }

    if(!res) {
        HALT_ERROR(NOT_ALLOWED, "ACL in unhandled memory area\n");
    }
    else if(res < 0) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL sanity check failed [%i]\n", res);
    }

    return size;
}

bool vmpu_region_get_for_box(uint8_t box_id, const MpuRegion * * const region, uint32_t * const count)
{
    if (!g_mpu_region_count) {
        HALT_ERROR(SANITY_CHECK_FAILED, "No available MPU regions.\r\n");
    }

    if (box_id < UVISOR_MAX_BOXES) {
        *count = g_mpu_box_region[box_id].count;
        *region = *count ? g_mpu_box_region[box_id].regions : NULL;
        return true;
    }
    return false;
}

MpuRegion * vmpu_region_find_for_address(uint8_t box_id, uint32_t address)
{
    int count;
    MpuRegion * region;

    count = g_mpu_box_region[box_id].count;
    region = g_mpu_box_region[box_id].regions;
    for (; count-- > 0; region++) {
        if ((region->start <= address) && (address < region->end)) {
            return region;
        }
    }

    return NULL;
}

/* Region management */

/* MPU access */

/* The K64F has 12 MPU regions, however, we use region 0 as the background
 * region with `UVISOR_TACL_BACKGROUND` as permissions.
 * Region 1 and 2 are used to unlock Application SRAM and Flash.
 * Therefore 9 MPU regions are available for user ACLs.
 * Region 3 and 4 are used to protect the current box stack and context.
 * This leaves 6 MPU regions for round robin scheduling:
 *
 *     12      <-- End of MPU regions, K64F_MPU_REGIONS_MAX
 * .---------.
 * |   11    |
 * |   ...   |
 * |    5    | <-- Box Pages, K64F_MPU_REGIONS_USER
 * +---------+
 * |    4    | <-- Box Context
 * |    3    | <-- Box Stack, K64F_MPU_REGIONS_STATIC
 * +---------+
 * |    2    | <-- Application SRAM unlock
 * |    1    | <-- Application Flash unlock
 * |    0    | <-- Background region
 * '---------'
 */
#define K64F_MPU_REGIONS_STATIC 3
#define K64F_MPU_REGIONS_USER 5
#define K64F_MPU_REGIONS_MAX 12

static uint8_t g_mpu_slot = K64F_MPU_REGIONS_STATIC;
static uint8_t g_mpu_priority[K64F_MPU_REGIONS_MAX];

void vmpu_mpu_init(void)
{
    /* Enable mem, bus and usage faults. */
    SCB->SHCSR |= 0x70000;
}

void vmpu_mpu_lock(void)
{
    /* MPU background region permission mask
     *   this mask must be set as last one, since the background region gives no
     *   executable privileges to neither user nor supervisor modes */
    MPU->RGDAAC[0] = UVISOR_TACL_BACKGROUND;
}

uint32_t vmpu_mpu_set_static_acl(uint8_t index, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion region;
    MPU_Region * mpu_region;
    uint32_t rounded_size;

    if (index >= K64F_MPU_REGIONS_STATIC) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_mpu_set_static_region: Invalid region index (%u)!\n", index);
        return 0;
    }

    rounded_size = vmpu_region_translate_acl(&region, start, size,
        acl, acl_hw_spec);

    mpu_region = (MPU_Region *) MPU->WORD[index];
    mpu_region->STARTADDR = region.start;
    mpu_region->ENDADDR = region.end;
    mpu_region->PERMISSIONS = region.config;
    mpu_region->CONTROL = 1;
    g_mpu_priority[index] = 255;

    return rounded_size;
}

void vmpu_mpu_invalidate(void)
{
    g_mpu_slot = K64F_MPU_REGIONS_STATIC;
    uint8_t slot = K64F_MPU_REGIONS_STATIC;
    while (slot < K64F_MPU_REGIONS_MAX) {
        /* We need to make sure that we disable an enabled MPU region before any
         * other modification, hence we cannot select the MPU region using the
         * region number field in the RBAR register. */
        ((MPU_Region *) MPU->WORD[slot])->CONTROL = 0;
        g_mpu_priority[slot] = 0;
        slot++;
    }
}

bool vmpu_mpu_push(const MpuRegion * const region, uint8_t priority)
{
    MPU_Region * mpu_region;
    if (!priority) priority = 1;

    const uint8_t start_slot = g_mpu_slot;
    uint8_t viable_slot = start_slot;

    do {
        if (g_mpu_priority[g_mpu_slot] < priority) {
            /* We can place this region in here. */
            mpu_region = (MPU_Region *) MPU->WORD[g_mpu_slot];
            mpu_region->STARTADDR = region->start;
            mpu_region->ENDADDR = region->end;
            mpu_region->PERMISSIONS = region->config;
            mpu_region->CONTROL = 1;

            g_mpu_priority[g_mpu_slot] = priority;

            if (++g_mpu_slot > K64F_MPU_REGIONS_MAX) {
                g_mpu_slot = K64F_MPU_REGIONS_STATIC;
            }
            return true;
        }
        viable_slot = g_mpu_slot;
        if (++g_mpu_slot > K64F_MPU_REGIONS_MAX) {
            g_mpu_slot = K64F_MPU_REGIONS_STATIC;
        }
    }
    while (g_mpu_slot != start_slot);

    /* We did not find a slot with a lower priority, so just take the next
     * position that does not have the highest priority. */
    mpu_region = (MPU_Region *) MPU->WORD[viable_slot];
    mpu_region->STARTADDR = region->start;
    mpu_region->ENDADDR = region->end;
    mpu_region->PERMISSIONS = region->config;
    mpu_region->CONTROL = 1;

    g_mpu_priority[viable_slot] = priority;

    return true;
}
