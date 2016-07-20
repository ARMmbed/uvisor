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
#ifndef __PAGE_ALLOCATOR_FAULTS_H__
#define __PAGE_ALLOCATOR_FAULTS_H__

#include "api/inc/page_allocator_exports.h"

/** Resets the fault count on a page. */
void page_allocator_reset_faults(uint8_t page);

/** Registers a fault on a page. */
void page_allocator_register_fault(uint8_t page);

/** @returns the number of faults on this page. */
uint32_t page_allocator_get_faults(uint8_t page);

/** Map an address to the start and end addresses of a page.
 * If the address is not part of any page, or the page does not belong to the
 * active box or box 0 an error is returned and `start` and `end` are invalid.
 *
 * @param address           the address to map
 * @param[out] start_addr   pointer to the start address value
 * @param[out] end_addr     pointer to the end address value
 * @param[out] page         pointer to the page index value
 * @returns Non-zero on failure with failure class `UVISOR_ERROR_CLASS_PAGE`. See `UVISOR_ERROR_PAGE_*`.
 */
int page_allocator_get_active_region_for_address(uint32_t address, uint32_t * start_addr, uint32_t * end_addr, uint8_t * page);

/** Callback for iterating over pages.
 * @param start_addr    the page start address
 * @param end_addr      the page end address
 * @param page          the page index
 * @retval 0            stop iteration after this callback.
 * @retval !0           continue iteration after this callback.
 */
typedef int (*PageAllocatorIteratorCallback)(uint32_t start_addr, uint32_t end_addr, uint8_t page);

/* Iterate over all pages belonging to the active box or box 0 and execute the callback.
 * @param callback  the function to execute on every page. May be NULL.
 * @return          number of active pages.
 */
uint8_t page_allocator_iterate_active_pages(PageAllocatorIteratorCallback callback);

#endif /* __PAGE_ALLOCATOR_FAULTS_H__ */
