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

/* This file contains the configuration-specific symbols. */
#include "configurations.h"

/* MPU region count */
#ifndef MPU_ACL_COUNT
#define MPU_ACL_COUNT 64
#endif/*MPU_ACL_COUNT*/

/* set default minimum region address alignment */
#ifndef ARMv7M_MPU_ALIGNMENT_BITS
#define ARMv7M_MPU_ALIGNMENT_BITS 5
#endif/*ARMv7M_MPU_ALIGNMENT_BITS*/

/* set default MPU region count */
#ifndef ARMv7M_MPU_REGIONS
#define ARMv7M_MPU_REGIONS 8
#endif/*ARMv7M_MPU_REGIONS*/

/* The ARMv7-M MPU has 8 MPU regions plus one background region.
 * Region 0 and 1 are used to unlock Application RAM and Flash.
 * In ARMv7-M MPU, region 2 is used to protect uVisor's own stack.
 * When switching into a secure box, region 3 is used to protect the boxes
 * stack and context.
 * If a box uses the page heap, the next region is used to protect it.
 * This leaves 3 to 5 MPU regions for round robin scheduling:
 *
 *      8      <-- End of MPU regions, ARMv7M_MPU_REGIONS_MAX
 * +---------+
 * |    7    |
 * |   ...   |
 * |  3/4/5  | <-- Start of round robin
 * +---------+
 * |   3/4   | <-- Optional Box Pages
 * +---------+
 * |    3    | <-- Secure Box Stack + Context, ARMv7M_MPU_REGIONS_STATIC
 * +---------+
 * |    2    | <-- uVisor's stack protection
 * |    1    | <-- Application SRAM unlock
 * |    0    | <-- Application Flash unlock
 * +---------+
 */
#define ARMv7M_MPU_REGIONS_STATIC 3
#define ARMv7M_MPU_REGIONS_MAX (ARMv7M_MPU_REGIONS)

/* MPU helper macros */
#define MPU_RBAR(region,addr)   (((uint32_t)(region))|MPU_RBAR_VALID_Msk|addr)
#define MPU_RBAR_RNR(addr)     (addr)

typedef struct
{
    MpuRegion * regions;
    uint32_t count;
} MpuRegionSlice;

static uint16_t g_mpu_region_count;
static MpuRegion g_mpu_region[MPU_ACL_COUNT];
static MpuRegionSlice g_mpu_box_region[UVISOR_MAX_BOXES];

static uint8_t g_mpu_slot = ARMv7M_MPU_REGIONS_STATIC;
static uint8_t g_mpu_priority[ARMv7M_MPU_REGIONS_MAX];

/* various MPU flags */
#define MPU_RASR_AP_PNO_UNO (0x00UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_UNO (0x01UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URO (0x02UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URW (0x03UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_UNO (0x05UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_URO (0x06UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_XN         (0x01UL<<MPU_RASR_XN_Pos)
#define MPU_RASR_CB_NOCACHE (0x00UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB_WRA  (0x01UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WT      (0x02UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB      (0x03UL<<MPU_RASR_B_Pos)
#define MPU_RASR_SRD(x)     (((uint32_t)(x))<<MPU_RASR_SRD_Pos)

int vmpu_is_region_size_valid(uint32_t size)
{
    /* Get the MSB of the size. */
    const int bits = vmpu_bits(size) - 1;
    if (bits < ARMv7M_MPU_ALIGNMENT_BITS || 29 < bits) {
        /* 2^5 == 32, which is the minimum region size. */
        /* 2^29 == 512M, which is the maximum region size. */
        return 0;
    }
    /* Compute the size from MSB. */
    const int bit_size = (1UL << bits);
    /* There is no round up. */

    /* We only care about an exact match! */
    return (bit_size == size);
}

uint32_t vmpu_round_up_region(uint32_t addr, uint32_t size)
{
    if (!vmpu_is_region_size_valid(size)) {
        /* Region size must be valid! */
        return 0;
    }
    /* Size is 2*N, can be used directly for alignment. */
    /* Create the LSB mask: Example 0x80 -> 0x7F. */
    const uint32_t mask = size - 1;
    /* Mask is 31 <= mask <= (1 << 29) - 1. */

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

uint8_t vmpu_region_bits(uint32_t size)
{
    uint8_t bits;

    bits = vmpu_bits(size) - 1;

    /* round up if needed */
    if((1UL << bits) != size) {
        bits++;
    }

    /* minimum region size is 32 bytes */
    if(bits < ARMv7M_MPU_ALIGNMENT_BITS) {
        bits = ARMv7M_MPU_ALIGNMENT_BITS;
    }

    assert(bits == UVISOR_REGION_BITS(size));
    return bits;
}

static uint32_t vmpu_map_acl(UvisorBoxAcl acl)
{
    uint32_t flags;
    UvisorBoxAcl acl_res;

    /* Map generic ACLs to internal ACLs. */
    if(acl & UVISOR_TACL_UWRITE) {
        acl_res =
            UVISOR_TACL_UREAD | UVISOR_TACL_UWRITE |
            UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
        flags = MPU_RASR_AP_PRW_URW;
    }
    else {
        if(acl & UVISOR_TACL_UREAD) {
            if(acl & UVISOR_TACL_SWRITE) {
                acl_res =
                    UVISOR_TACL_UREAD |
                    UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
                flags = MPU_RASR_AP_PRW_URO;
            }
            else {
                acl_res =
                    UVISOR_TACL_UREAD |
                    UVISOR_TACL_SREAD;
                flags = MPU_RASR_AP_PRO_URO;
            }
        }
        else {
            if(acl & UVISOR_TACL_SWRITE) {
                acl_res =
                    UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
                flags = MPU_RASR_AP_PRW_UNO;
            }
            else {
                if(acl & UVISOR_TACL_SREAD) {
                    acl_res = UVISOR_TACL_SREAD;
                    flags = MPU_RASR_AP_PRO_UNO;
                }
                else {
                    acl_res = 0;
                    flags = MPU_RASR_AP_PNO_UNO;
                }
            }
        }
    }

    /* Handle code-execute flag. */
    if( acl & (UVISOR_TACL_UEXECUTE | UVISOR_TACL_SEXECUTE) ) {
        /* Can't distinguish between user & supervisor execution. */
        acl_res |= UVISOR_TACL_UEXECUTE | UVISOR_TACL_SEXECUTE;
    }
    else {
        flags |= MPU_RASR_XN;
    }

    /* Check if we meet the expected ACLs. */
    if( acl_res != (acl & UVISOR_TACL_ACCESS) ) {
        HALT_ERROR(SANITY_CHECK_FAILED, "inferred ACL's (0x%04X) don't match exptected ACL's (0x%04X)\n", acl_res, (acl & UVISOR_TACL_ACCESS));
    }

    return flags;
}

uint32_t vmpu_region_translate_acl(MpuRegion * const region, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    uint32_t config, bits, mask, size_rounded, subregions;

    /* verify region alignment */
    bits = vmpu_region_bits(size);
    size_rounded = 1UL << bits;

    if(size_rounded != size) {
        if ((acl & (UVISOR_TACL_SIZE_ROUND_UP | UVISOR_TACL_SIZE_ROUND_DOWN)) == 0) {
            HALT_ERROR(SANITY_CHECK_FAILED, "box size (%i) not rounded, rounding disabled (rounded=%i)\n", size, size_rounded);
        }

        if (acl & UVISOR_TACL_SIZE_ROUND_DOWN) {
            bits--;
            if(bits < ARMv7M_MPU_ALIGNMENT_BITS) {
                HALT_ERROR(SANITY_CHECK_FAILED, "region size (%i) can't be rounded down\n", size);
            }
            size_rounded = 1UL << bits;
        }
    }

    /* check for correctly aligned base address */
    mask = size_rounded - 1;

    if(start & mask) {
        HALT_ERROR(SANITY_CHECK_FAILED, "start address 0x%08X and size (%i) are inconsistent\n", start, size);
    }

    /* map generic ACL's to internal ACL's */
    config = vmpu_map_acl(acl);

    /* calculate subregions from hw-specific ACL */
    subregions = (acl_hw_spec << MPU_RASR_SRD_Pos) & MPU_RASR_SRD_Msk;

    /* enable region & add size */
    region->config = config | MPU_RASR_ENABLE_Msk | ((uint32_t) (bits - 1) << MPU_RASR_SIZE_Pos) | subregions;
    region->start = start;
    region->end = start + size_rounded;
    region->acl = acl;

    return size_rounded;
}

uint32_t vmpu_region_add_static_acl(uint8_t box_id, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion * region;
    MpuRegionSlice * box;
    uint32_t rounded_size;

    if(g_mpu_region_count >= MPU_ACL_COUNT) {
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
    for (uint8_t slot = 0; slot < ARMv7M_MPU_REGIONS_STATIC; ++slot) {
        MPU->RNR = slot;
        uint32_t rasr = MPU->RASR;

        /* Is the region enabled? */
        if (!(rasr & MPU_RASR_ENABLE_Msk)) {
            continue;
        }

        uint32_t rbar = MPU->RBAR;
        uint32_t size = (1UL << (((rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos) + 1UL));
        uint32_t start = rbar & ~(size - 1);
        uint32_t end = start + size;
        /* Check entire region if no subregion is disabled. */
        if (!(rasr & MPU_RASR_SRD_Msk)) {
            /* Test that the buffer is fully contained in the region. */
            if (vmpu_value_in_range(start, end, start_addr) && vmpu_value_in_range(start, end, end_addr)) {
                return true;
            }
        } else {
            /* Check each subregion separately. */
            uint32_t sub_size = size / 8UL;
            for (uint8_t ii = 0; ii < 8UL; ii++) {
                /* Is this subregion enabled (= NOT disabled)? */
                if (!(rasr & (1UL << (ii + MPU_RASR_SRD_Pos)))) {
                    uint32_t sub_start = start + sub_size * ii;
                    uint32_t sub_end = sub_start + sub_size;
                    /* Test that the buffer is fully contained in the region. */
                    if (vmpu_value_in_range(sub_start, sub_end, start_addr) && vmpu_value_in_range(sub_start, sub_end, end_addr)) {
                        return true;
                    }
                }
            }
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
    uint32_t aligment_mask;

    /* Disable the MPU. */
    MPU->CTRL = 0;
    /* Check MPU region alignment using region number zero. */
    MPU->RNR = 0;
    MPU->RBAR = MPU_RBAR_ADDR_Msk;
    aligment_mask = ~MPU->RBAR;
    MPU->RBAR = 0;

    /* Enable mem, bus and usage faults. */
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk) |
                  (SCB_SHCSR_BUSFAULTENA_Msk) |
                  (SCB_SHCSR_MEMFAULTENA_Msk);

    /* show basic mpu settings */
    DPRINTF("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
    DPRINTF("MPU.ALIGNMENT=0x%08X\r\n", aligment_mask);
    DPRINTF("MPU.ALIGNMENT_BITS=%i\r\n", vmpu_bits(aligment_mask));

    /* Perform sanity checks. */
    if (ARMv7M_MPU_REGIONS != ((MPU->TYPE >> MPU_TYPE_DREGION_Pos) & 0xFF)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ARMv7M_MPU_REGIONS is inconsistent with actual region count.\n\r");
    }
    if (ARMv7M_MPU_ALIGNMENT_BITS != vmpu_bits(aligment_mask)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "ARMv7M_MPU_ALIGNMENT_BITS are inconsistent with actual MPU alignment\n\r");
    }
}

void vmpu_mpu_lock(void)
{
    /* DMB to ensure MPU update after all transfer to memory completed */
    __DMB();

    /* Finally enable the MPU. */
    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

    /* DSB & ISB to ensure subsequent data & instruction transfers are using updated MPU settings */
    __DSB();
    __ISB();
}

uint32_t vmpu_mpu_set_static_acl(uint8_t index, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    MpuRegion region;
    uint32_t rounded_size;

    if (index >= ARMv7M_MPU_REGIONS_STATIC) {
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_mpu_set_static_region: Invalid region index (%u)!\n", index);
        return 0;
    }

    rounded_size = vmpu_region_translate_acl(&region, start, size,
        acl, acl_hw_spec);

    /* apply RASR & RBAR */
    MPU->RBAR = MPU_RBAR(index, region.start);
    MPU->RASR = region.config;
    g_mpu_priority[index] = 255;

    return rounded_size;
}

void vmpu_mpu_invalidate(void)
{
    g_mpu_slot = ARMv7M_MPU_REGIONS_STATIC;
    uint8_t slot = ARMv7M_MPU_REGIONS_STATIC;
    while (slot < ARMv7M_MPU_REGIONS_MAX) {
        /* We need to make sure that we disable an enabled MPU region before any
         * other modification, hence we cannot select the MPU region using the
         * region number field in the RBAR register. */
        MPU->RNR = slot;
        MPU->RASR = 0;
        MPU->RBAR = 0;
        g_mpu_priority[slot] = 0;
        slot++;
    }
}

bool vmpu_mpu_push(const MpuRegion * const region, uint8_t priority)
{
    if (!priority) priority = 1;

    const uint8_t start_slot = g_mpu_slot;
    uint8_t viable_slot = start_slot;

    do {
        if (++g_mpu_slot >= ARMv7M_MPU_REGIONS_MAX) {
            g_mpu_slot = ARMv7M_MPU_REGIONS_STATIC;
        }

        if (g_mpu_priority[g_mpu_slot] < priority) {
            /* We can place this region in here. */
            MPU->RBAR = MPU_RBAR(g_mpu_slot, region->start);
            MPU->RASR = region->config;
            g_mpu_priority[g_mpu_slot] = priority;
            return true;
        }
        viable_slot = g_mpu_slot;
    }
    while (g_mpu_slot != start_slot);

    /* We did not find a slot with a lower priority, so just take the next
     * position that does not have the highest priority. */
    MPU->RBAR = MPU_RBAR(viable_slot, region->start);
    MPU->RASR = region->config;
    g_mpu_priority[viable_slot] = priority;

    return true;
}
