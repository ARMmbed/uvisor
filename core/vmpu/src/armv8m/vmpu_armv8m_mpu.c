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
#include "vmpu.h"
#include "vmpu_mpu.h"

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

uint32_t vmpu_region_translate_acl(MpuRegion * const region, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return 0;
}

uint32_t vmpu_region_add_static_acl(uint8_t box_id, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return 0;
}

bool vmpu_region_get_for_box(uint8_t box_id, const MpuRegion * * const region, uint32_t * const count)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return false;
}

MpuRegion * vmpu_region_find_for_address(uint8_t box_id, uint32_t address)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return NULL;
}

void vmpu_mpu_init(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

void vmpu_mpu_lock(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

uint32_t vmpu_mpu_set_static_acl(uint8_t index, uint32_t start, uint32_t size, UvisorBoxAcl acl, uint32_t acl_hw_spec)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return 0;
}

void vmpu_mpu_invalidate(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

bool vmpu_mpu_push(const MpuRegion * const region, uint8_t priority)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return false;
}
