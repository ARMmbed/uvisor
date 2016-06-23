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
uint32_t g_page_fault_table[UVISOR_PAGE_TABLE_MAX_COUNT];

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
    /* Check that the page actually belongs to this box or box 0. */
    if ((g_page_owner_table[p] != 0) && (g_page_owner_table[p] != box_id)) {
        return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
    }
    /* Compute the page start and end address */
    *start_addr = (uint32_t) g_page_heap_start + g_page_size * p;
    *end_addr = *start_addr + g_page_size;
    *page = p;

    return UVISOR_ERROR_PAGE_OK;
}
