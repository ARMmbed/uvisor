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

/* By default a maximum of 32 pages are allowed */
#ifndef UVISOR_PAGE_TABLE_MAX_COUNT
#   define UVISOR_PAGE_TABLE_MAX_COUNT ((size_t)32)
#endif
/* The page box_id is the box id which is 8bit large  */
typedef uint8_t page_owner_t;
/* Maps the page to the owning box handle */
static page_owner_t page_owner_table[UVISOR_PAGE_TABLE_MAX_COUNT];
/* define a unused value for the page table */
#define UVISOR_PAGE_UNUSED ((page_owner_t)(-1))

/* contains the configured page size */
static uint32_t page_size;
/* Points to the beginning of the page heap */
static const void* page_heap_start;
/* Points to the end of the page heap */
static const void* page_heap_end;
/* Contains the number of free pages */
static uint8_t page_count_free;
/* Contains the total number of available pages */
static uint8_t page_count_total;

/* Must be equal to page size for MPU alignment on ARMv7-M */
#define UVISOR_PAGE_ALIGNMENT (page_size)
/* #define UVISOR_PAGE_ALIGNMENT (32) */


void page_allocator_init(void *heap_start, void *heap_end, uint32_t config_page_size)
{
    /* make sure the UVISOR_PAGE_UNUSED is definitely NOT a valid box id! */
    if (vmpu_is_box_id_valid(UVISOR_PAGE_UNUSED))
        HALT_ERROR(SANITY_CHECK_FAILED,
            "UVISOR_PAGE_UNUSED (%u) must not be a valid box id!\n",
            UVISOR_PAGE_UNUSED);

    /* page size has to be at least 32B */
    config_page_size = config_page_size < 32 ? 32 : config_page_size;
    /* align the page size to the next largest power of two for ARMv7-M only */
    const size_t mask = (1 << (vmpu_bits(config_page_size) - 1)) - 1;
    page_size = (config_page_size + mask) & ~mask;
    page_size = 16*1024;

    uint32_t start = (uint32_t)heap_start;
    /* round up to the nearest page aligned memory address */
    start += UVISOR_PAGE_ALIGNMENT - 1;
    start &= ~(UVISOR_PAGE_ALIGNMENT - 1);
    /* this is the page heap start address */
    page_heap_start = (void*)start;

    /* how many pages can we fit in here? */
    page_count_total = ((uint32_t)heap_end - start) / page_size;
    /* clamp page count to table size */
    if (page_count_total > UVISOR_PAGE_TABLE_MAX_COUNT) page_count_total = UVISOR_PAGE_TABLE_MAX_COUNT;
    page_count_free = page_count_total;
    /* remember the end of the heap */
    page_heap_end = page_heap_start + page_count_free * page_size;

    DPRINTF("uvisor_page_init:\n.page_heap start 0x%08x\n.page_heap end   0x%08x\n.page_heap available %ukB split into %u pages of %ukB\n\n",
            (unsigned int) page_heap_start,
            (unsigned int) page_heap_end,
            (unsigned int) (page_count_free * page_size / 1024),
            (unsigned int) page_count_total,
            (unsigned int) (page_size / 1024));

    size_t page = 0;
    for (; page < page_count_total; page++) {
        page_owner_table[page] = UVISOR_PAGE_UNUSED;
    }
}

int page_allocator_malloc(UvisorPageTable *const table)
{
    /* check if we can fulfill the requested page size */
    if (table->page_size != page_size) {
        DPRINTF("uvisor_page_malloc: FAIL: Requested page size %uB is not the configured page size %uB!\n\n", table->page_size, page_size);
        return UVISOR_ERROR_PAGE_INVALID_PAGE_SIZE;
    }
    /* check if we have enough pages available */
    if (table->page_count > page_count_free) {
        DPRINTF("uvisor_page_malloc: FAIL: Cannot serve %u pages with only %u free pages!\n\n", table->page_count, page_count_free);
        return UVISOR_ERROR_PAGE_OUT_OF_MEMORY;
    }

    /* get the calling box id */
    page_owner_t box_id = g_active_box;
    DPRINTF("uvisor_page_malloc: Requesting %u pages with size %uB for box %u\n", table->page_count, table->page_size, box_id);

    /* update the free pages count */
    page_count_free -= table->page_count;
    size_t pages_required = table->page_count;
    void* *page_origins = table->page_origins;

    /* iterate through the page table and find the empty pages */
    size_t page = 0;
    for (; (page < page_count_total) && pages_required; page++) {
        /* if the page is unused, it's entry is UVISOR_PAGE_UNUSED (not NULL!) */
        if (page_owner_table[page] == UVISOR_PAGE_UNUSED) {
            /* marry this page to the box id */
            page_owner_table[page] = box_id;
            /* get the pointer to the page */
            void* ptr = (void*)page_heap_start + page * page_size;
            /* zero the entire page before handing it out */
            memset(ptr, 0, page_size);
            /* write the pages address to the table in the first page */
            *page_origins++ = ptr;
            /* one less page required */
            pages_required--;
            DPRINTF("uvisor_page_malloc: Found an empty page 0x%08x entry at index %u\n", (unsigned int) ptr, page);
        }
    }
    DPRINTF("uvisor_page_malloc: %u free pages remaining.\n\n", page_count_free);

    return UVISOR_ERROR_PAGE_OK;
}

int page_allocator_free(const UvisorPageTable *const table)
{
    if (page_count_free == page_count_total) {
        DPRINTF("uvisor_page_free: FAIL: There are no pages to free!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE;
    }
    if (table->page_size != page_size) {
        DPRINTF("uvisor_page_free: FAIL: Requested page size %uB is not the configured page size %uB!\n\n", table->page_size, page_size);
        return UVISOR_ERROR_PAGE_INVALID_PAGE_SIZE;
    }
    if (table->page_count == 0) {
        DPRINTF("uvisor_page_free: FAIL: Pointer table is empty!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE;
    }
    if (table->page_count > (page_count_total - page_count_free)) {
        DPRINTF("uvisor_page_free: FAIL: Pointer table too large!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE;
    }

    /* get the calling box id */
    page_owner_t box_id = g_active_box;
    int table_size = table->page_count;

    /* contains the bit mask of all the pages we need to free */
    uint32_t free_mask[(UVISOR_PAGE_TABLE_MAX_COUNT + 31) / 32] = {0};
    /* iterate over the table and validate each pointer */
    void* const *page = table->page_origins;
    for (; table_size > 0; page++, table_size--) {
        /* range check the returned pointer */
        if (*page < page_heap_start || *page >= page_heap_end) {
            DPRINTF("uvisor_page_free: FAIL: Pointer 0x%08x does not belong to any page!\n\n", (unsigned int) *page);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN;
        }
        /* compute the index for the pointer */
        size_t page_index = (*page - page_heap_start) / page_size;
        /* check if the page belongs to the caller */
        if (page_owner_table[page_index] == box_id) {
            free_mask[page_index/32] |= (1 << (page_index & 31));
        }
        /* abort if the page doesn't belong to the caller */
        else if (page_owner_table[page_index] == UVISOR_PAGE_UNUSED) {
            DPRINTF("uvisor_page_free: FAIL: Page %u is not allocated!\n\n", page_index);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
        }
        else {
            DPRINTF("uvisor_page_free: FAIL: Page %u is not owned by box %u!\n\n", page_index, box_id);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
        }
    }
    /* we now have validated the pages in the table.
     * all pages that need to be freed are in the free_mask */

    /* iterate over the bits in the free mask and actually free the pages */
    size_t count = 0;
    for (; count < page_count_total; count++) {
        if (free_mask[count/32] & (1 << (count & 31))) {
            page_owner_table[count] = UVISOR_PAGE_UNUSED;
            DPRINTF("uvisor_page_free: Freeing page at index %u\n", count);
        }
    }
    /* we have freed some pages */
    page_count_free += table->page_count;

    DPRINTF("uvisor_page_free: %u free pages available.\n\n", page_count_free);
    return UVISOR_ERROR_PAGE_OK;
}
