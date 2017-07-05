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
#include "page_allocator_faults.h"
#include "vmpu.h"
#include "vmpu_mpu.h"

/* This file contains the configuration-specific symbols. */
#include "configurations.h"

/* SAU region count */
#ifndef SAU_ACL_COUNT
#define SAU_ACL_COUNT 64
#endif/*SAU_ACL_COUNT*/

/* set default SAU region count */
#ifndef ARMv8M_SAU_REGIONS
#define ARMv8M_SAU_REGIONS 8
#endif/*ARMv8M_SAU_REGIONS*/

/* The v8-M SAU has 8 regions.
 * Region 0, 1, 2, 3 are used to unlock Application SRAM and Flash.
 * Therefore 4 SAU regions are available for user ACLs.
 * Region 4 and 4 are used to protect the current box stack and context.
 * This leaves 6 SAU regions for round robin scheduling:
 *
 *      8      <-- End of SAU regions, V8M_SAU_REGIONS_MAX
 * .---------.
 * |    7    |
 * |    6    |
 * |    5    | <-- Box Pages, V8M_SAU_REGIONS_USER
 * +---------+
 * |    4    | <-- Box Context
 * +---------+
 * |    3    | <-- Application SRAM unlock
 * |    2    | <-- Application Flash after uVisor unlock
 * |    1    | <-- Application Flash Non-Secure-Callable in uVisor
 * |    0    | <-- Application Flash before uVisor unlock
 * '---------'
 */
#define ARMv8M_SAU_REGIONS_STATIC 4
#define ARMv8M_SAU_REGIONS_MAX (ARMv8M_SAU_REGIONS)

/* SAU helper macros */
#define SAU_RLAR(config,addr)   ((config) | (((addr) - 1UL) & ~(32UL - 1UL)))

typedef struct
{
    MpuRegion * regions;
    uint32_t count;
} MpuRegionSlice;

static uint16_t g_mpu_region_count;
static MpuRegion g_mpu_region[SAU_ACL_COUNT];
static MpuRegionSlice g_mpu_box_region[UVISOR_MAX_BOXES];

static uint8_t g_mpu_slot = ARMv8M_SAU_REGIONS_STATIC;
static uint8_t g_mpu_priority[ARMv8M_SAU_REGIONS_MAX];

int vmpu_is_region_size_valid(uint32_t size)
{
    /* Align size to 32B. */
    uint32_t const masked_size = size & ~31UL;
    if (masked_size < 32 || (1 << 29) < masked_size) {
        /* 2^5 == 32, which is the minimum region size. */
        /* 2^29 == 512M, which is the maximum region size. */
        return 0;
    }
    /* There is no rounding, we only care about an exact match. */
    return (masked_size == size);
}

uint32_t vmpu_round_up_region(uint32_t addr, uint32_t size)
{
    if (!vmpu_is_region_size_valid(size)) {
        /* Region size must be valid. */
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

static uint32_t vmpu_map_acl(UvisorBoxAcl acl)
{
    uint32_t flags = 0;

    if (acl & UVISOR_TACL_UACL) {
        /* If any user access is required, enable the NS region. */
        flags = SAU_RLAR_ENABLE_Msk;
    }
    /* Every other kind of access is not supported by hardware. */
    return flags;
}

uint32_t vmpu_region_translate_acl(MpuRegion * const region, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
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

    region->config = vmpu_map_acl(acl) | (acl_hw_spec & SAU_RLAR_NSC_Msk);
    region->start = start;
    region->end = start + size;
    region->acl = acl;

    return size;
}

uint32_t vmpu_region_add_static_acl(uint8_t box_id, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion * region;
    MpuRegionSlice * box;
    uint32_t rounded_size;

    if(g_mpu_region_count >= SAU_ACL_COUNT) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_add_static_acl: No more regions to allocate!\n");
    }

    if (!vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_add_static_acl: The box id (%u) is out of range.\n", box_id);
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

bool vmpu_region_get_for_box(uint8_t box_id, const MpuRegion * * const region, uint32_t * const count)
{
    if (!g_mpu_region_count) {
        *count = 0;
        return false;
        // HALT_ERROR(SANITY_CHECK_FAILED, "No available SAU regions.\r\n");
    }

    if (box_id < UVISOR_MAX_BOXES) {
        *count = g_mpu_box_region[box_id].count;
        *region = *count ? g_mpu_box_region[box_id].regions : NULL;
        return true;
    }
    return false;
}

static bool value_in_range(size_t start, size_t end, size_t value)
{
    return start <= value && value < end;
}

MpuRegion * vmpu_region_find_for_address(uint8_t box_id, uint32_t address)
{
    int count;
    MpuRegion * region;

    count = g_mpu_box_region[box_id].count;
    region = g_mpu_box_region[box_id].regions;
    for (; count-- > 0; region++) {
        if (value_in_range(region->start, region->end, address)) {
            return region;
        }
    }

    return NULL;
}


static bool vmpu_region_is_ns(uint32_t rlar)
{
    return (rlar & SAU_RLAR_NSC_Msk) == 0 &&
           (rlar & SAU_RLAR_ENABLE_Msk) == SAU_RLAR_ENABLE_Msk;
}

static bool vmpu_buffer_access_is_ok_static(uint32_t start_addr, uint32_t end_addr)
{
    /* NOTE: Buffers are not allowed to span more than 1 region. If they do
     * span more than one region, access will be denied. */

    /* Search through all static regions programmed into the SAU, until we find
     * a region that contains both the start and end of our buffer. */
    for (uint8_t slot = 0; slot < ARMv8M_SAU_REGIONS_STATIC; ++slot) {
        SAU->RNR = slot;
        uint32_t rlar = SAU->RLAR;

        /* Is the region NS? */
        if (!vmpu_region_is_ns(rlar)) {
            continue;
        }

        uint32_t rbar = SAU->RBAR;
        uint32_t start = rbar & ~0x1F; /* The bottom 5 bits are not part of the base address. */
        uint32_t end = rlar | 0x1F; /* The bottom 5 bits are not part of the limit. */

        /* Test that the buffer is fully contained in the region. */
        if (value_in_range(start, end, start_addr) && value_in_range(start, end, end_addr)) {
            return true;
        }
    }

    return false;
}

bool vmpu_buffer_access_is_ok(int box_id, const void * addr, size_t size)
{
    uint32_t start_addr = (uint32_t) UVISOR_GET_NS_ALIAS(addr);
    uint32_t end_addr = start_addr + size - 1;

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

    assert(start_addr <= end_addr);
    if (start_addr > end_addr) {
        /* We couldn't determine the end of the buffer. */
        return false;
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
    return value_in_range(region->start, region->end, end_addr) &&
           vmpu_region_is_ns(region->config);
}

/* SAU access */

void vmpu_mpu_init(void)
{
    SAU->CTRL = 0;

    if (ARMv8M_SAU_REGIONS != ((SAU->TYPE >> SAU_TYPE_SREGION_Pos) & 0xFF)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ARMv8M_SAU_REGIONS is inconsistent with actual region count.\n\r");
    }
}

void vmpu_mpu_lock(void)
{
    SAU->CTRL = SAU_CTRL_ENABLE_Msk;
}

uint32_t vmpu_mpu_set_static_acl(uint8_t index, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion region;
    uint32_t rounded_size;

    if (index >= ARMv8M_SAU_REGIONS_STATIC) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_mpu_set_static_region: Invalid region index (%u)!\n", index);
        return 0;
    }

    rounded_size = vmpu_region_translate_acl(&region, start, size, acl, acl_hw_spec);

    SAU->RNR = index;
    SAU->RBAR = region.start;
    SAU->RLAR = SAU_RLAR(region.config, region.end);

    g_mpu_priority[index] = 255;

    return rounded_size;
}

void vmpu_mpu_invalidate(void)
{
    g_mpu_slot = ARMv8M_SAU_REGIONS_STATIC;
    uint8_t slot = ARMv8M_SAU_REGIONS_STATIC;
    while (slot < ARMv8M_SAU_REGIONS_MAX) {
        /* We need to make sure that we disable an enabled SAU region before any
         * other modification, hence we cannot select the SAU region using the
         * region number field in the RBAR register. */
        SAU->RNR = slot;
        SAU->RBAR = 0;
        SAU->RLAR = 0;
        g_mpu_priority[slot] = 0;
        slot++;
    }
}

static void vmpu_sau_add_region(const MpuRegion * const region, uint8_t slot, uint8_t priority)
{
    SAU->RNR = slot;
    SAU->RBAR = region->start;
    SAU->RLAR = SAU_RLAR(region->config, region->end);
    g_mpu_priority[slot] = priority;
}

bool vmpu_mpu_push(const MpuRegion * const region, uint8_t priority)
{
    if (!priority) priority = 1;

    const uint8_t start_slot = g_mpu_slot;

    do {
        g_mpu_slot++;
        if (g_mpu_slot >= ARMv8M_SAU_REGIONS_MAX) {
            g_mpu_slot = ARMv8M_SAU_REGIONS_STATIC;
        }

        if (g_mpu_priority[g_mpu_slot] < priority) {
            /* We can place this region in here. */
            vmpu_sau_add_region(region, g_mpu_slot, priority);
            return true;
        }
    }
    while (g_mpu_slot != start_slot);

    g_mpu_slot++;
    if (g_mpu_slot >= ARMv8M_SAU_REGIONS_MAX) {
        g_mpu_slot = ARMv8M_SAU_REGIONS_STATIC;
    }

    /* We did not find a slot with a lower priority, so just take the next
     * position that does not have the highest priority. */
    vmpu_sau_add_region(region, g_mpu_slot, priority);

    return true;
}
