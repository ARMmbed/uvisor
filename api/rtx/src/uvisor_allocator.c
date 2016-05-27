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
#include "uvisor_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include "uvisor-lib/uvisor-lib.h"
#include "rt_TypeDef.h"
#include "rt_Memory.h"
#include <string.h>

#define DPRINTF(...) {};
/* #define DPRINTF(fmt, ...) printf(fmt, ## __VA_ARGS__) */

/* Internal structure currently only contains the page table */
typedef struct {
    UvisorPageTable table;
} UvisorAllocatorInternal;

static inline UvisorPageTable * table(UvisorAllocator allocator) {
    return &(((UvisorAllocatorInternal *) allocator)->table);
}

UvisorAllocator uvisor_allocator_create_with_pool(
    void * mem,
    size_t bytes)
{
    UvisorAllocatorInternal * allocator = mem;
    /* Signal that this is non-page allocated memory */
    allocator->table.page_size = bytes;
    allocator->table.page_count = 0;
    /* The MEMP structure must be placed AFTER table.page_origins[0] !!! */
    size_t offset = offsetof(UvisorAllocatorInternal, table.page_origins) + sizeof(void *);
    /* Create MEMP structure inside the memory */
    if (rt_init_mem(mem + offset, bytes - offset)) {
        /* Abort if failed */
        DPRINTF("uvisor_allocator_create_with_pool: MEMP allocator creation failed\n\n");
        return NULL;
    }
    /* Remember the MEMP pointer though */
    allocator->table.page_origins[0] = mem + offset;
    DPRINTF("uvisor_allocator_create_with_pool: Created MEMP allocator %p with offset %d\n\n", mem + offset, offset);
    return allocator;
}

UvisorAllocator uvisor_allocator_create_with_pages(
    size_t size,
    size_t maximum_malloc_size)
{
    /* Additional overhead for each block */
    const size_t block_overhead = 2 * sizeof(MEMP);
    size_t page_count = 0;

    /* Calcutate the required total size considering page size and overhead */
    for (int request = size; request > 0; request -= UVISOR_PAGE_SIZE, page_count++)
    {
        /* Adjust the requested and total sizes */
        request += block_overhead;
        size += block_overhead;
    }
    DPRINTF("uvisor_allocator_create_with_pages: Requesting %u pages for at least %uB\n", page_count, size);

    /* Compute the maximum allocation within our blocks */
    size_t maximum_allocation_size = UVISOR_PAGE_SIZE - block_overhead;
    /* If the required maximum allocation is larger than we can provide, abort */
    if (maximum_malloc_size > maximum_allocation_size) {
        DPRINTF("uvisor_allocator_create_with_pages: Maximum allocation request %uB is larger then available %uB\n\n", maximum_malloc_size, maximum_allocation_size);
        return NULL;
    }

    /* Compute the require memory size for the page table */
    size_t allocator_type_size = sizeof(UvisorAllocatorInternal);
    /* Add size for each additional page */
    allocator_type_size += (page_count - 1) * sizeof(void *);
    /* Allocate this much memory */
    UvisorAllocatorInternal * const allocator = malloc(allocator_type_size);
    /* If malloc failed, abort */
    if (allocator == NULL) {
        DPRINTF("uvisor_allocator_create_with_pages: allocator_t failed to be allocated!\n\n");
        return NULL;
    }

    /* Prepare the page table */
    allocator->table.page_size = UVISOR_PAGE_SIZE;
    allocator->table.page_count = page_count;
    /* Get me some pages */
    if (uvisor_page_malloc((UvisorPageTable*)&(allocator->table))) {
        DPRINTF("uvisor_allocator_create_with_pages: Not enough free pages available!\n\n");
        free(allocator);
        return NULL;
    }

    /* Initialize a MEMP structure in all pages */
    for(size_t ii=0; ii < page_count; ii++)
    {
        /* Add each page as a pool */
        rt_init_mem(allocator->table.page_origins[ii], UVISOR_PAGE_SIZE);
        DPRINTF("uvisor_allocator_create_with_pages: Created MEMP allocator %p with offset %d\n", allocator->table.page_origins[ii], 0);
    }
    DPRINTF("\n");
    /* Aaaand across the line */
    return (UvisorAllocator)allocator;
}

int uvisor_allocator_destroy(
    UvisorAllocator allocator)
{
    DPRINTF("uvisor_allocator_destroy: Destroying MEMP allocator at %p\n", table(allocator)->page_origins[0]);

    /* Check if we are working on statically allocated memory */
    UvisorAllocatorInternal * alloc = (UvisorAllocatorInternal * const) allocator;
    if (alloc->table.page_count == 0) {
        DPRINTF("uvisor_allocator_destroy: %p is not page-backed memory, not freeing!\n", allocator);
        return -1;
    }

    /* Free all pages */
    if (uvisor_page_free(&(alloc->table))) {
        DPRINTF("uvisor_allocator_destroy: Unable to free pages!\n\n");
        return -1;
    }

    /* Free the allocator structure */
    free(allocator);

    DPRINTF("\n");
    return 0;
}

void * uvisor_malloc(
    UvisorAllocator allocator,
    size_t size)
{
    size_t index = 0;
    do {
        /* Search in this page */
        void * mem = rt_alloc_mem(table(allocator)->page_origins[index], size);
        /* Return if we found something */
        if (mem) {
            DPRINTF("uvisor_malloc: Found %4uB in page %u at %p\n", size, index, mem);
            return mem;
        }
        /* Otherwise, go to the next page */
        index++;
    } /* Continue search if more pages are available */
    while(index < table(allocator)->page_count);
    DPRINTF("uvisor_malloc: Out of memory in allocator %p \n", allocator);
    /* We found nothing */
    return NULL;
}

void * uvisor_realloc(
    UvisorAllocator allocator,
    void * ptr,
    size_t new_size)
{
    /* THIS IS A NAIVE IMPLEMENTATION, which always allocates new memory,
       and copies the memory, then freeing the old memory */

    /* Allocate new memory */
    void * new_ptr = uvisor_malloc(allocator, new_size);
    /* If memory allocation failed, abort */
    if (new_ptr == NULL) return NULL;

    /* Passing NULL as ptr is legal, realloc acts as malloc then */
    if (ptr) {
        /* Get the size of the ptr memory */
        size_t size = ((MEMP *) ((uint32_t)ptr - sizeof(MEMP)))->len;
        /* Copy the memory to the new location, min(new_size, size) */
        memcpy(new_ptr, ptr, new_size < size ? new_size : size);
        /* Free the previous memory */
        uvisor_free(allocator, ptr);
    }
    return new_ptr;
}

void uvisor_free(
    UvisorAllocator allocator,
    void * ptr)
{
    size_t index = 0;
    do {
        /* Search in this page */
        int ret = rt_free_mem(table(allocator)->page_origins[index], ptr);
        /* Return if free was successful */
        if (ret == 0) {
            DPRINTF("uvisor_free: Freed %p in page %u.\n", ptr, index);
            return;
        }
        /* Otherwise, go to the next page */
        index++;
    } /* Continue search if more pages are available */
    while(index < table(allocator)->page_count);
    DPRINTF("uvisor_free: %p not found in allocator %p!\n", ptr, allocator);
    /* We found nothing */
    return;
}
