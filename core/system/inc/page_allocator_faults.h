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

/** Check if a box is allowed to access a address range.
 * Note that the address range must be contained inside one page.
 *
 * @param box_id       the id of the box to query for
 * @param start_addr   the start address of the range
 * @param end_addr     the end address of the range
 * @retval
 *  - `UVISOR_ERROR_PAGE_OK`  range is contained in one page owned by the box id
 *  - `UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER` range is spread over multiple pages or owners
 *  - `UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN` range is not outside of in page heap
 */
int page_allocator_check_range_for_box(int box_id, uint32_t start_addr, uint32_t end_addr);

/** Map an address to the start and end addresses of a page.
 * If the address is not part of any page, or the page does not belong to the
 * active box or box 0 an error is returned and `start`, `end` and `page` are invalid.
 *
 * @param address           the address to locate in the page heap
 * @param[out] start_addr   the start address of the found page
 * @param[out] end_addr     the end address of the found page
 * @param[out] page         the physical page index of the found page
 * @returns Non-zero on failure with failure class `UVISOR_ERROR_CLASS_PAGE`. See `UVISOR_ERROR_PAGE_*`.
 */
int page_allocator_get_active_region_for_address(uint32_t address, uint32_t * start_addr, uint32_t * end_addr, uint8_t * page);

/** Map an address to an 8-bit page mask.
 * If the address is not part of any page, or the page does not belong to the
 * active box or box 0 an error is returned and `mask`, `index` and `page` are invalid.
 *
 * Example with 12 pages:
 *  - map:  0b11111111'11110000'00000000'00000000
 *      1. address maps to page 9: mask=0xFF, index=3, page=9
 *      2. address maps to page 3: mask=0xF0, index=2, page=3
 *
 * @param address           the address to locate in the page heap
 * @param[out] mask         the 8-bit aligned page mask containing the found page
 * @param[out] index        the position of the page mask counting from the LSB!
 * @param[out] page         the physical page index of the found page
 * @returns Non-zero on failure with failure class `UVISOR_ERROR_CLASS_PAGE`. See `UVISOR_ERROR_PAGE_*`.
 */
int page_allocator_get_active_mask_for_address(uint32_t address, uint8_t * mask, uint8_t * index, uint8_t * page);

typedef enum
{
    /** The iterator will start at the lowest page counting upwards. */
    PAGE_ALLOCATOR_ITERATOR_DIRECTION_FORWARD = 1,
    /** The iterator will start at the highest page counting downwards. */
    PAGE_ALLOCATOR_ITERATOR_DIRECTION_BACKWARD = -1,
} PageAllocatorIteratorDirection;

/** Callback for iterating over pages.
 *
 * Note that the page index always increments relative to iteration direction.
 * This means if you iterate backwards, page index 0 corresponds the the
 * physically highest page and "increments" downwards.
 *
 * @param start_addr    the page start address
 * @param end_addr      the page end address
 * @param page          the page index relative to iteration direction
 * @retval 0            stop iteration after this callback.
 * @retval !0           continue iteration after this callback.
 */
typedef int (*PageAllocatorIteratorCallback)(uint32_t start_addr, uint32_t end_addr, uint8_t page);

/* Iterate over all pages belonging to the active box or box 0 and execute the callback.ss
 *
 * @param callback  the function to execute on every page. Returns number of active pages if `NULL`.
 * @param direction forward or backwards direction.
 * @return          number of callbacks, or if `NULL` passed as callback, number of active pages.
 */
uint8_t page_allocator_iterate_active_pages(PageAllocatorIteratorCallback callback, PageAllocatorIteratorDirection direction);

/** Callback for iterating over 8-bit page masks.
 *
 * Note that the mask and the mask index are always aligned to the 8-bit boundary!
 * This means that iterating forward starts indexing at the LSB of the map, and
 * iterating  backwards starts indexing at the MSB of the map.
 * The LSB is the lowest byte containing page owner bits.
 *
 * Example with 12 pages:
 *  - map:  0b11111111'11110000
 *      1. forward iteration callbacks:
 *          1. mask=0xF0, index=0
 *          2. mask=0xFF, index=1
 *      2. backward iteration callbacks:
 *          1. mask=0xFF, index=0
 *          2. mask=0xF0, index=1
 *
 * @param mask          the page owner mask
 * @param index         the index of the page mask
 * @retval 0            stop iteration after this callback.
 * @retval !0           continue iteration after this callback.
 */
typedef int (*PageAllocatorIteratorMaskCallback)(uint8_t mask, uint8_t index);

/* Iterate over all page masks belonging to the active box or box 0 and execute the callback.
 *
 * @param callback  the function to execute on every page mask. Returns number of active page masks if `NULL`.
 * @param direction forward or backwards direction.
 * @return          number of callbacks, or if `NULL` passed as callback, number of active page masks.
 */
uint8_t page_allocator_iterate_active_page_masks(PageAllocatorIteratorMaskCallback callback, PageAllocatorIteratorDirection direction);

#endif /* __PAGE_ALLOCATOR_FAULTS_H__ */
