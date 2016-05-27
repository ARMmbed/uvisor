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
#ifndef __UVISOR_LIB_ALLOCATOR_H__
#define __UVISOR_LIB_ALLOCATOR_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Contains the allocator data and backing page table */
typedef void * UvisorAllocator;

/** Create an allocator in an existing pool without using pages. */
UvisorAllocator uvisor_allocator_create_with_pool(
    void * mem,
    size_t bytes);

/** Create an allocator using pages.
 * @param total_size          the minimal total size of the heap
 * @param maximum_malloc_size the largest size to be allocated in one chunk
 */
UvisorAllocator uvisor_allocator_create_with_pages(
    size_t total_size,
    size_t maximum_malloc_size);

/** Destroy the allocator and frees the backing pages. */
int uvisor_allocator_destroy(
    UvisorAllocator allocator);

/** Drop-in for `malloc` */
void* uvisor_malloc(
    UvisorAllocator allocator,
    size_t size);

/** Drop-in for `realloc` */
void* uvisor_realloc(
    UvisorAllocator allocator,
    void * ptr,
    size_t size);

/** Drop-in for `free` */
void uvisor_free(
    UvisorAllocator allocator,
    void * ptr);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* __UVISOR_LIB_ALLOCATOR_H__ */
