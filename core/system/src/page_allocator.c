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

/* This file can be compiled externally to provide the page allocator algorithm
   for devices NOT supported by uVisor. For this purpose this file is copied as
   is into uvisor-mbed-lib and compiled by the target build system. */
#if defined(UVISOR_PRESENT) && (UVISOR_PRESENT == 1)
#include <uvisor.h>
#include "page_allocator.h"
#include "api/inc/vmpu_unpriv_access_exports.h"
#include "mpu/vmpu.h"
#include "halt.h"
#include "context.h"
#else
#include "uvisor-lib/uvisor-lib.h"
#define DPRINTF(...) {}
#define g_active_box 0
#define vmpu_is_box_id_valid(...) 0
#define HALT_ERROR(id, ...) DPRINTF(__VA_ARGS__)
static int vmpu_is_region_size_valid(uint32_t size)
{
    /* Minimal 32B alignment */
    return ((size & ~31) == size);
}
static uint32_t vmpu_round_up_region(uint32_t addr, uint32_t size)
{
    if (!vmpu_is_region_size_valid(size)) {
        return 0;
    }
    const uint32_t mask = size - 1;
    return (addr + mask) & ~mask;
}
#endif

/* By default a maximum of 16 pages are allowed. This can only
   be decided by the porting engineer for the current platform! */
#ifndef UVISOR_PAGE_TABLE_MAX_COUNT
#define UVISOR_PAGE_TABLE_MAX_COUNT ((size_t)16)
#endif
/* The page box_id is the box id which is 8bit large  */
typedef uint8_t page_owner_t;
/* Maps the page to the owning box handle */
static page_owner_t g_page_owner_table[UVISOR_PAGE_TABLE_MAX_COUNT];
/* define a unused value for the page table */
#define UVISOR_PAGE_UNUSED ((page_owner_t)(-1))

/* contains the configured page size */
static uint32_t g_page_size;
/* Points to the beginning of the page heap */
static const void * g_page_heap_start;
/* Points to the end of the page heap */
static const void * g_page_heap_end;
/* Contains the number of free pages */
static uint8_t g_page_count_free;
/* Contains the total number of available pages */
static uint8_t g_page_count_total;


void page_allocator_init(void * const heap_start, void * const heap_end, uint32_t page_size)
{
    /* Make sure the UVISOR_PAGE_UNUSED is definitely NOT a valid box id! */
    if (vmpu_is_box_id_valid(UVISOR_PAGE_UNUSED)) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "UVISOR_PAGE_UNUSED (%u) must not be a valid box id!\n",
            UVISOR_PAGE_UNUSED);
    }

    if ((page_size < 1024) || !vmpu_is_region_size_valid(page_size)) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "Page size (%uB) not supported by this platform!\n",
            page_size);
    }
    g_page_size = page_size;

    uint32_t start = vmpu_round_up_region((uint32_t) heap_start, page_size);
    if (start == 0) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "Page heap start address 0x%08x cannot be aligned with page size %uB!\n",
            (unsigned int) heap_start,
            (unsigned int) page_size);
    }
    /* This is the page heap start address */
    g_page_heap_start = (void *) start;

    /* How many pages can we fit in here? */
    if ((start + page_size) > (uint32_t) heap_end) {
        g_page_count_total = 0;
    }
    else {
        g_page_count_total = ((uint32_t) heap_end - start) / g_page_size;
    }
    /* Clamp page count to table size */
    if (g_page_count_total > UVISOR_PAGE_TABLE_MAX_COUNT) {
        g_page_count_total = UVISOR_PAGE_TABLE_MAX_COUNT;
    }
    g_page_count_free = g_page_count_total;
    /* Remember the end of the heap */
    g_page_heap_end = g_page_heap_start + g_page_count_free * g_page_size;

    DPRINTF("uvisor_page_init:\n.page_heap start 0x%08x\n.page_heap end   0x%08x\n.page_heap available %ukB split into %u pages of %ukB\n\n",
            (unsigned int) g_page_heap_start,
            (unsigned int) g_page_heap_end,
            (unsigned int) (g_page_count_free * g_page_size / 1024),
            (unsigned int) g_page_count_total,
            (unsigned int) (page_size / 1024));

    size_t page = 0;
    for (; page < g_page_count_total; page++) {
        g_page_owner_table[page] = UVISOR_PAGE_UNUSED;
    }
}

int page_allocator_malloc(UvisorPageTable * const table)
{
    uint32_t pages_required = vmpu_unpriv_uint32_read((uint32_t) &(table->page_count));
    uint32_t page_size = vmpu_unpriv_uint32_read((uint32_t) &(table->page_size));
    /* Check if the user even wants any pages */
    if (pages_required == 0) {
        DPRINTF("uvisor_page_malloc: FAIL: No pages requested!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_COUNT;
    }
    /* Check if we can fulfill the requested page size */
    if (page_size != g_page_size) {
        DPRINTF("uvisor_page_malloc: FAIL: Requested page size %uB is not the configured page size %uB!\n\n", page_size, g_page_size);
        return UVISOR_ERROR_PAGE_INVALID_PAGE_SIZE;
    }
    /* Check if we have enough pages available */
    if (pages_required > g_page_count_free) {
        DPRINTF("uvisor_page_malloc: FAIL: Cannot serve %u pages with only %u free pages!\n\n", pages_required, g_page_count_free);
        return UVISOR_ERROR_PAGE_OUT_OF_MEMORY;
    }

    /* Get the calling box id */
    const page_owner_t box_id = g_active_box;
    DPRINTF("uvisor_page_malloc: Requesting %u pages with size %uB for box %u\n", pages_required, page_size, box_id);

    /* Update the free pages count */
    g_page_count_free -= pages_required;
    /* point to the first entry in the table */
    void * * page_table = table->page_origins;

    /* Iterate through the page table and find the empty pages */
    size_t page = 0;
    for (; (page < g_page_count_total) && pages_required; page++) {
        /* If the page is unused, it's entry is UVISOR_PAGE_UNUSED (not NULL!) */
        if (g_page_owner_table[page] == UVISOR_PAGE_UNUSED) {
            /* Marry this page to the box id */
            g_page_owner_table[page] = box_id;
            /* Get the pointer to the page */
            void * ptr = (void *) g_page_heap_start + page * g_page_size;
            /* Zero the entire page before handing it out */
            memset(ptr, 0, g_page_size);
            /* Write the pages address to the table in the first page */
            vmpu_unpriv_uint32_write((uint32_t) page_table, (uint32_t) ptr);
            page_table++;
            /* One less page required */
            pages_required--;
            DPRINTF("uvisor_page_malloc: Found an empty page 0x%08x entry at index %u\n", (unsigned int) ptr, page);
        }
    }
    DPRINTF("uvisor_page_malloc: %u free pages remaining.\n\n", g_page_count_free);

    return UVISOR_ERROR_PAGE_OK;
}

int page_allocator_free(const UvisorPageTable *const table)
{
    if (g_page_count_free == g_page_count_total) {
        DPRINTF("uvisor_page_free: FAIL: There are no pages to free!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE;
    }
    uint32_t page_count = vmpu_unpriv_uint32_read((uint32_t) &(table->page_count));
    uint32_t page_size = vmpu_unpriv_uint32_read((uint32_t) &(table->page_size));
    if (page_size != g_page_size) {
        DPRINTF("uvisor_page_free: FAIL: Requested page size %uB is not the configured page size %uB!\n\n", page_size, g_page_size);
        return UVISOR_ERROR_PAGE_INVALID_PAGE_SIZE;
    }
    if (page_count == 0) {
        DPRINTF("uvisor_page_free: FAIL: Pointer table is empty!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_COUNT;
    }
    if (page_count > (unsigned) (g_page_count_total - g_page_count_free)) {
        DPRINTF("uvisor_page_free: FAIL: Pointer table too large!\n\n");
        return UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE;
    }

    /* Get the calling box id */
    const page_owner_t box_id = g_active_box;
    int table_size = page_count;

    /* Contains the bit mask of all the pages we need to free */
    uint32_t free_mask[(UVISOR_PAGE_TABLE_MAX_COUNT + 31) / 32] = {0};
    /* Iterate over the table and validate each pointer */
    void * const * page_table = table->page_origins;

    for (; table_size > 0; page_table++, table_size--) {
        void *page = (void *) vmpu_unpriv_uint32_read((uint32_t) page_table);
        /* Range check the returned pointer */
        if (page < g_page_heap_start || page >= g_page_heap_end) {
            DPRINTF("uvisor_page_free: FAIL: Pointer 0x%08x does not belong to any page!\n\n", (unsigned int) page);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN;
        }
        /* Compute the index for the pointer */
        size_t page_index = (page - g_page_heap_start) / g_page_size;
        /* Check if the page belongs to the caller */
        if (g_page_owner_table[page_index] == box_id) {
            free_mask[page_index/32] |= (1 << (page_index & 31));
        }
        /* Abort if the page doesn't belong to the caller */
        else if (g_page_owner_table[page_index] == UVISOR_PAGE_UNUSED) {
            DPRINTF("uvisor_page_free: FAIL: Page %u is not allocated!\n\n", page_index);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
        }
        else {
            DPRINTF("uvisor_page_free: FAIL: Page %u is not owned by box %u!\n\n", page_index, box_id);
            return UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER;
        }
    }
    /* We now have validated the pages in the table.
     * All pages that need to be freed are in the free_mask */

    /* Iterate over the bits in the free mask and actually free the pages */
    size_t count = 0;
    for (; count < g_page_count_total; count++) {
        if (free_mask[count/32] & (1 << (count & 31))) {
            g_page_owner_table[count] = UVISOR_PAGE_UNUSED;
            DPRINTF("uvisor_page_free: Freeing page at index %u\n", count);
        }
    }
    /* We have freed some pages */
    g_page_count_free += page_count;

    DPRINTF("uvisor_page_free: %u free pages available.\n\n", g_page_count_free);
    return UVISOR_ERROR_PAGE_OK;
}
