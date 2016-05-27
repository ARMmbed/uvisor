/*
 * Copyright (c) 2015-2016, ARM Limited, All Rights Reserved
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
#include "uvisor-lib/uvisor-lib.h"

#if !(defined(UVISOR_PRESENT) && (UVISOR_PRESENT == 1))

#include <string.h>

/* Note: This file is not included in the uVisor release library. Instead, the
 *       host OS needs to compile it separately if a platform does not support
 *       uVisor (but uVisor API header files are still used). */

void page_allocator_init(void * heap_start, void * heap_end, uint32_t page_size);
int page_allocator_malloc(UvisorPageTable * const table);
int page_allocator_free(const UvisorPageTable * const table);

extern uint32_t __end__[];      /* __heap_start */
extern uint32_t __HeapLimit[];  /* __heap_end   */

extern uint32_t __StackLimit[];   /* bottom of stack */

UvisorBoxIndexOS * __uvisor_ps;

static void box_index_init(void *box_bss, uint32_t heap_size)
{
    const uint32_t index_size = sizeof(UvisorBoxIndexOS);
    /* Adjust size for overhead of box index */
    heap_size -= index_size;

    /* The box index is at the beginning of the bss section */
    UvisorBoxIndexOS *const indexOS = box_bss;
    memset(box_bss, 0, index_size);
    /* Initialize user context */
    indexOS->index.ctx = NULL;
    /* Initialize process heap */
    indexOS->index.process_heap = box_bss;
    indexOS->index.process_heap_size = heap_size;
    /* Active heap pointer is NULL */
    indexOS->index.active_heap = NULL;

    /* Cache the box id */
    indexOS->index.box_id = 0;

    /* Point the mutex pointer to the data */
    indexOS->mutex = &(indexOS->mutex_data);
    /* Set the id to NULL */
    indexOS->mutex_id = NULL;

    /* Set the index */
    __uvisor_ps = indexOS;
}

/* uVisor hook for unsupported platforms */
void uvisor_malloc_init(void)
{
    /* get the main heap size from the linker script */
    uint32_t heap_size = ((uint32_t) __HeapLimit -
                          (uint32_t) __end__);
    /* Main heap size is aligned to page boundaries n*UVISOR_PAGE_SIZE */
    uint32_t heap_start = (uint32_t) __StackLimit - heap_size;
    /* align the start address of the main heap to a page boundary */
    heap_start &= ~(UVISOR_PAGE_SIZE - 1);
    /* adjust the heap size to the new heap start address */
    heap_size = (uint32_t) __StackLimit - heap_start;

    /* page heap now extends from the previous main heap start address
     * to the new main heap start address */
    page_allocator_init(__end__, (void *) heap_start, UVISOR_PAGE_SIZE);
    box_index_init((void *) heap_start, heap_size);
    return;
}


int uvisor_page_malloc(UvisorPageTable *const table)
{
    return page_allocator_malloc(table);
}

int uvisor_page_free(const UvisorPageTable *const table)
{
    return page_allocator_free(table);
}

#include "source/page_allocator.c_inc"

#endif
