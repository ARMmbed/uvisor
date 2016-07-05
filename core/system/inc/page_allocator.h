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
#ifndef __PAGE_ALLOCATOR_H__
#define __PAGE_ALLOCATOR_H__

#include "api/inc/page_allocator_exports.h"
#include "page_allocator_config.h"

/* Initialize the page allocator.
 *
 * The most important argument to this function is the page size. It determines
 * the alignment of the start and end address, where the alignment size is the
 * page size for ARMv7-M.
 * The start address is rounded upwards, whereas the end address is rounded
 * downwards to the next aligned border.
 * This means that not all of the memory between start and end address can be
 * used by the heap address, due to adhering to alignment restrictions.
 *
 * @param heap_start[in] Start address of the page heap
 * @param heap_end[in]   End address of the page heap
 * @param page_size      Pointer to page size for the allocator
 */
void page_allocator_init(void * const heap_start, void * const heap_end, const uint32_t * const page_size);

/* Allocate a number of requested pages with the requested page size.
 * @param table.page_size[in]     Must be equal to the current page size
 * @param table.page_count[in]    The number of pages to be allocated
 * @param table.page_origins[out] Pointers to the page origins. The table must be large enough to hold page_count entries.
 * @returns Non-zero on failure with failure class `UVISOR_ERROR_CLASS_PAGE`. See `UVISOR_ERROR_PAGE_*`.
 */
int page_allocator_malloc(UvisorPageTable * const table);

/* Free the pages associated with the table, only if it passes validation.
 * @returns Non-zero on failure with failure class `UVISOR_ERROR_CLASS_PAGE`. See `UVISOR_ERROR_PAGE_*`.
 */
int page_allocator_free(const UvisorPageTable * const table);

/* Map an address to a page index.
 * @return page index or `UVISOR_PAGE_UNUSED` if address does not belong to page heap.
 */
uint8_t page_allocator_get_page_from_address(uint32_t address);

/* Contains the configured page size. */
extern uint32_t g_page_size;
/* Points to the beginning of the page heap. */
extern const void * g_page_heap_start;
/* Contains the total number of available pages. */
extern uint8_t g_page_count_total;
/* Maps the page to the owning box handle. */
extern page_owner_t g_page_owner_table[];

#endif /* __PAGE_ALLOCATOR_H__ */
