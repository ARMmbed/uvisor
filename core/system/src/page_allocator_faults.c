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
#include "page_allocator.h"
#include "page_allocator_faults.h"
#include "page_allocator_config.h"
#include "context.h"

/* Maps the number of faults to a page. */
uint32_t g_page_fault_table[UVISOR_PAGE_MAX_COUNT];

void page_allocator_reset_faults(uint8_t page)
{
    if (page < g_page_count_total) {
        g_page_fault_table[page] = 0;
    }
}

void page_allocator_register_fault(uint8_t page)
{
    if (page < g_page_count_total) {
        g_page_fault_table[page]++;
    }
}

uint32_t page_allocator_get_faults(uint8_t page)
{
    if (page < g_page_count_total) {
        return g_page_fault_table[page];
    }
    return 0;
}

int page_allocator_get_active_region_for_address(uint32_t address, uint32_t * start_addr, uint32_t * end_addr, uint8_t * page)
{
    const page_owner_t box_id = g_active_box;

    /* Compute the page id. */
    uint8_t p = page_allocator_get_page_from_address(address);
    if (p == UVISOR_PAGE_UNUSED) {
        /* This address does not correspond to any page. */
        return UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN;
    }
    /* Then check if the page is set. */
    if (!page_allocator_map_get(g_page_owner_map[box_id], p)) {
        return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
    }
    /* Compute the page start and end address */
    *start_addr = (uint32_t) g_page_heap_start + g_page_size * p;
    *end_addr = *start_addr + g_page_size;
    *page = p;

    return UVISOR_ERROR_PAGE_OK;
}

int page_allocator_get_active_mask_for_address(uint32_t address, uint8_t * mask, uint8_t * index, uint8_t * page)
{
    const page_owner_t box_id = g_active_box;
    /* Compute the page id. */
    uint8_t p = page_allocator_get_page_from_address(address);
    if (p == UVISOR_PAGE_UNUSED) {
        /* This address does not correspond to any page. */
        return UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN;
    }
    /* Then check if the page is set. */
    if (!page_allocator_map_get(g_page_owner_map[box_id], p)) {
        return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
    }
    *page = p;
    /* Compute the page mask and index. */
    p += g_page_map_shift;
    *mask = (uint8_t) (g_page_owner_map[box_id][p / 32] >> ((p % 32) & ~7));
    *index = p / 8;

    return UVISOR_ERROR_PAGE_OK;
}

uint8_t page_allocator_iterate_active_pages(PageAllocatorIteratorCallback callback, PageAllocatorIteratorDirection direction)
{
    uint8_t ii, index, count = 0;
    uint32_t start_addr, end_addr;
    const page_owner_t box_id = g_active_box;

    /* Iterate over all pages. */
    for (ii = 0; ii < g_page_count_total; ii++) {
        if (direction < 0) {
            index = (g_page_count_total - 1) - ii;
        } else {
            index = ii;
        }
        if (page_allocator_map_get(g_page_owner_map[box_id], index)) {
            count++;
            if (callback) {
                /* Compute start and end addresses. */
                start_addr = (uint32_t) g_page_heap_start + g_page_size * index;
                end_addr = start_addr + g_page_size;
                /* Call the callback. */
                if (!callback(start_addr, end_addr, ii)) {
                    return count;
                }
            }
        }
    }

    return count;
}

uint8_t page_allocator_iterate_active_page_masks(PageAllocatorIteratorMaskCallback callback, PageAllocatorIteratorDirection direction)
{
    uint8_t ii, index, mask, count = 0;
    const page_owner_t box_id = g_active_box;
    const uint8_t page_count_octets = ((g_page_count_total + 7) / 8);

    for (ii = 0; ii < page_count_octets; ii++) {
        if (direction < 0) {
            index = UVISOR_PAGE_MAP_COUNT * 4 - 1 - ii;
        } else {
            index = (UVISOR_PAGE_MAP_COUNT * 4 - page_count_octets) + ii;
        }
        mask = (uint8_t) (g_page_owner_map[box_id][index / 4] >> ((index % 4) * 8));
        if (mask) {
            count++;
            if (callback) {
                /* Call the callback. */
                if (!callback(mask, ii)) {
                    return count;
                }
            }
        }
    }

    return count;
}
