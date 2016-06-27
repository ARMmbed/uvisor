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
