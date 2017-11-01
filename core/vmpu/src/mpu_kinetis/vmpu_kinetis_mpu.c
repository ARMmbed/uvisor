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
#include "page_allocator_faults.h"
#include "vmpu.h"
#include "vmpu_mpu.h"
#include "vmpu_kinetis_aips.h"
#include "vmpu_kinetis_mem.h"

/* This file contains the configuration-specific symbols. */
#include "configurations.h"

/* The K64F has 12 MPU regions, however, we use region 0 as the background
 * region for debugging purposes only.
 * Regions 1 and 2 are used to create a stack guard for uVisor's own stack
 * Region 3 and 4 are used to unlock Application SRAM and Flash.
 * Therefore 7 MPU regions are available for user ACLs.
 * Region 5 and 6 are used to protect the current box stack and context.
 * This leaves 5 MPU regions for round robin scheduling:
 *
 *     12      <-- End of MPU regions, K64F_MPU_REGIONS_MAX
 * .---------.
 * |   11    |
 * |   ...   |
 * |    7    | <-- Box Pages, K64F_MPU_REGIONS_USER
 * +---------+
 * |    6    | <-- Box Context
 * |    5    | <-- Box Stack, K64F_MPU_REGIONS_STATIC
 * +---------+
 * |    4    | <-- Application SRAM unlock
 * |    3    | <-- Application Flash unlock
 * |    2    | <-- Background region (stack start to 0xFFFFFFFF)
 * |    1    | <-- Background region (0 to stack guard)
 * |    0    | <-- Background region (for debugger)
 * '---------'
 */
#define K64F_MPU_REGIONS_STATIC 5
#define K64F_MPU_REGIONS_USER 7
#define K64F_MPU_REGIONS_MAX 12

typedef struct
{
    MpuRegion * regions;
    uint32_t count;
} MpuRegionSlice;

static uint16_t g_mpu_region_count;
static MpuRegion g_mpu_region[UVISOR_MAX_ACLS];
static MpuRegionSlice g_mpu_box_region[UVISOR_MAX_BOXES];

static uint8_t g_mpu_slot = K64F_MPU_REGIONS_STATIC;
static uint8_t g_mpu_priority[K64F_MPU_REGIONS_MAX];

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

    /* check for maximum box ID */
    if (!vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL add: The box ID is out of range (%u).\n", box_id);
    }

    /* check for alignment to 32 bytes */
    if(start & 0x1F) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL start address is not aligned [0x%08X]\n", start);
    }

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
        HALT_ERROR(SANITY_CHECK_FAILED, "No available MPU regions.\n");
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
        if (vmpu_value_in_range(region->start, region->end, address)) {
            return region;
        }
    }

    return NULL;
}

/* Region management */

static bool vmpu_buffer_access_is_ok_static(uint32_t start_addr, uint32_t end_addr)
{
    /* NOTE: Buffers are not allowed to span more than 1 region. If they do
     * span more than one region, access will be denied. */

    /* Search through all static regions programmed into the MPU, until we find
     * a region that contains both the start and end of our buffer. */
    for (uint8_t slot = 0; slot < K64F_MPU_REGIONS_STATIC; ++slot) {
        MPU_Region * mpu_region = (MPU_Region *) MPU->WORD[slot];
        uint32_t start = mpu_region->STARTADDR;
        uint32_t end = mpu_region->ENDADDR;

        /* Test that the buffer is fully contained in the region. */
        if (vmpu_value_in_range(start, end, start_addr) && vmpu_value_in_range(start, end, end_addr)) {
            return true;
        }
    }

    return false;
}

bool vmpu_buffer_access_is_ok(int box_id, const void * addr, size_t size)
{
    uint32_t start_addr = (uint32_t) addr;
    uint32_t end_addr = start_addr + size - 1;

    assert(start_addr <= end_addr);
    if (start_addr > end_addr) {
        /* We couldn't determine the end of the buffer. */
        return false;
    }

    /* NOTE: Buffers are not allowed to span more than 1 region. If they do
     * span more than one region, access will be denied. */

    if (box_id != 0) {
        /* Check the public box as well as the specified box, since public box
         * memories are accessible by all boxes. */
        if (vmpu_buffer_access_is_ok(0, addr, size)) {
            return true;
        }
    } else {
        /* Check static regions. */
        if (vmpu_buffer_access_is_ok_static(start_addr, end_addr)) {
            return true;
        }
    }

    /* Check if addr range lies in AIPS */
    if (vmpu_fault_find_acl_aips(box_id, start_addr, size)) {
        return true;
    }

    /* Check if addr range lies in page heap. */
    int error = page_allocator_check_range_for_box(box_id, start_addr, end_addr);
    if (error == UVISOR_ERROR_PAGE_OK) {
        return true;
    } else if (error != UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN) {
        return false;
    }

    MpuRegion * region = vmpu_region_find_for_address(box_id, start_addr);
    if (!region) {
        /* No region contained the start of the buffer. */
        return false;
    }

    /* If the end address is also within the region, and the region is NS
     * accessible, then access to the buffer is OK. */
    return vmpu_value_in_range(region->start, region->end, end_addr);
}

/* MPU access */

void vmpu_mpu_init(void)
{
    /* Enable mem, bus and usage faults. */
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk) |
                  (SCB_SHCSR_BUSFAULTENA_Msk) |
                  (SCB_SHCSR_MEMFAULTENA_Msk);
}


// Creates the stack guard using MPU regions 1 & 2
static void vmpu_mpu_set_background_region()
{
    MPU_Region * mpu_region = NULL;

    mpu_region = (MPU_Region *) MPU->WORD[1];
    mpu_region->STARTADDR = 0x00000000;
    // the region ends at __uvisor_stack_start_boundary__ not including
    mpu_region->ENDADDR = (((uint32_t) &__uvisor_stack_start_boundary__) - 1);
    mpu_region->PERMISSIONS = (MPU_WORD_M0UM(MPU_RGDn_WORD2_MxUM_NONE) | MPU_WORD_M0SM(MPU_RGDn_WORD2_MxSM_RW));
    mpu_region->CONTROL = 1;

    mpu_region = (MPU_Region *) MPU->WORD[2];
    mpu_region->STARTADDR = ((uint32_t) &__uvisor_stack_start__);
    mpu_region->ENDADDR = 0xFFFFFFFF;
    mpu_region->PERMISSIONS = (MPU_WORD_M0UM(MPU_RGDn_WORD2_MxUM_NONE) | MPU_WORD_M0SM(MPU_RGDn_WORD2_MxSM_RW));
    mpu_region->CONTROL = 1;
}
void vmpu_mpu_lock(void)
{
    vmpu_mpu_set_background_region();
    /* DMB to ensure MPU update after all transfer to memory completed */
    __DMB();

    /* MPU background region permission mask
     *   this mask must be set as last one, since the background region gives no
     *   executable privileges to neither user nor supervisor modes */
    MPU->RGDAAC[0] = MPU_WORD_M0UM(MPU_RGDn_WORD2_MxUM_NONE)  |
        MPU_WORD_M0SM(MPU_RGDn_WORD2_MxSM_AS_UM)              |
        MPU_WORD_M1UM(MPU_RGDn_WORD2_MxUM_READ |
            MPU_RGDn_WORD2_MxUM_WRITE |
            MPU_RGDn_WORD2_MxUM_EXECUTE)                      |
        MPU_WORD_M1SM(MPU_RGDn_WORD2_MxSM_RWX);

    /* DSB & ISB to ensure subsequent data & instruction transfers are using updated MPU settings */
    __DSB();
    __ISB();
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
