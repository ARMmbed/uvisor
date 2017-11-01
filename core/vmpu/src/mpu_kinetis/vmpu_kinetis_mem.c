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
#include "vmpu_mpu.h"
#include "context.h"
#include "vmpu_kinetis_mem.h"
#include "page_allocator_faults.h"

/* This is the iterator callback for inserting all page heap ACLs into to the
 * MPU during `vmpu_mem_switch()`. */
static int vmpu_mem_push_page_acl_iterator(uint32_t start_addr, uint32_t end_addr, uint8_t page)
{
    (void) page;
    MpuRegion region = {.start = start_addr, .end = end_addr, .config = 0x1E};
    /* We only continue if we have not wrapped around the end of the MPU regions yet. */
    return vmpu_mpu_push(&region, 100);
}

int vmpu_mem_push_page_acl(uint32_t start_addr, uint32_t end_addr)
{
    /* Check that start and end address are aligned to 32-byte. */
    if (start_addr & 0x1F || end_addr & 0x1F) {
        return -1;
    }

    vmpu_mem_push_page_acl_iterator(start_addr, end_addr, g_active_box);

    return 0;
}

void vmpu_mem_switch(uint8_t src_box, uint8_t dst_box)
{
    uint32_t dst_count;
    const MpuRegion * region;

    vmpu_mpu_invalidate();

    if(dst_box) {
        /* Update target box first to make target stack available. */
        vmpu_region_get_for_box(dst_box, &region, &dst_count);

        while (dst_count-- && vmpu_mpu_push(region++, 255)) ;
    }

    page_allocator_iterate_active_pages(vmpu_mem_push_page_acl_iterator, PAGE_ALLOCATOR_ITERATOR_DIRECTION_FORWARD);
}

void vmpu_mem_init(void)
{
    /* enable read access to unsecure flash regions
     * - allow execution
     * - give read access to ENET DMA bus master and generic (UART, I2C, etc.)
     *   DMA bus master */
    vmpu_mpu_set_static_acl(3, FLASH_ORIGIN, (uint32_t) __uvisor_config.secure_end - FLASH_ORIGIN,
        UVISOR_TACL_SREAD |
        UVISOR_TACL_SEXECUTE |
        UVISOR_TACL_UREAD |
        UVISOR_TACL_UEXECUTE |
        UVISOR_TACL_USER,
        MPU_WORD_M2UM(MPU_RGDn_WORD2_MxUM_READ) | MPU_WORD_M3UM(MPU_RGDn_WORD2_MxUM_READ)
    );

    /* rest of SRAM, accessible to mbed
     * - non-executable for uvisor
     * - give read/write access to ENET DMA bus master and generic (UART, I2C, etc.)
     *   DMA bus master */
    vmpu_mpu_set_static_acl(4, (uint32_t) __uvisor_config.page_end,
        (uint32_t) __uvisor_config.sram_end - (uint32_t) __uvisor_config.page_end,
        UVISOR_TACL_SREAD |
        UVISOR_TACL_SWRITE |
        UVISOR_TACL_UREAD |
        UVISOR_TACL_UWRITE |
        UVISOR_TACL_UEXECUTE |
        UVISOR_TACL_USER,
        MPU_WORD_M2UM(MPU_RGDn_WORD2_MxUM_READ | MPU_RGDn_WORD2_MxUM_WRITE) |
        MPU_WORD_M3UM(MPU_RGDn_WORD2_MxUM_READ | MPU_RGDn_WORD2_MxUM_WRITE)
    );
}
