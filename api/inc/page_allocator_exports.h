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
#ifndef __UVISOR_API_PAGE_ALLOCATOR_EXPORTS_H__
#define __UVISOR_API_PAGE_ALLOCATOR_EXPORTS_H__

#include "api/inc/halt_exports.h"
#include <stdint.h>
#include <stddef.h>

/** @addtogroup page_allocator
 * @{
 */

/** @name Error return codes
 * @{
 */
/** @brief No error occurred. */
#define UVISOR_ERROR_PAGE_OK                    (0)
/** @brief The page heap does not have enough free memory to serve your request. */
#define UVISOR_ERROR_PAGE_OUT_OF_MEMORY         (UVISOR_ERROR_CLASS_PAGE + 1)
/** @brief The page table is not correct. */
#define UVISOR_ERROR_PAGE_INVALID_PAGE_TABLE    (UVISOR_ERROR_CLASS_PAGE + 2)
/** @brief The page table contains an invalid page size. */
#define UVISOR_ERROR_PAGE_INVALID_PAGE_SIZE     (UVISOR_ERROR_CLASS_PAGE + 3)
/** @brief The page table contains at least one invalid page origin. */
#define UVISOR_ERROR_PAGE_INVALID_PAGE_ORIGIN   (UVISOR_ERROR_CLASS_PAGE + 4)
/** @brief The page table contains pages of which you are not the owner. */
#define UVISOR_ERROR_PAGE_INVALID_PAGE_OWNER    (UVISOR_ERROR_CLASS_PAGE + 5)
/** @brief The page table contains more pages than you own in total. */
#define UVISOR_ERROR_PAGE_INVALID_PAGE_COUNT    (UVISOR_ERROR_CLASS_PAGE + 6)
/** @} */

/** @cond UVISOR_INTERNAL */
UVISOR_EXTERN const uint32_t __uvisor_page_size;
/** @endcond */

/** This user-allocated structure is used to communicate allocation and free
 * requests to and from the page allocator.
 */
typedef struct {
    /** The page size in bytes. Must be multiple of `uvisor_get_page_size()`! */
    uint32_t page_size;
    /** The number of pages in the table. */
    uint32_t page_count;
    /** Table of pointers to the origin of each page. */
    void * page_origins[1];
} UvisorPageTable;

/** @} */

#endif /* __UVISOR_API_PAGE_ALLOCATOR_EXPORTS_H__ */
