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
#ifndef __VMPU_MPU_H__
#define __VMPU_MPU_H__

#include "api/inc/uvisor_exports.h"
#include "api/inc/vmpu_exports.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t start;     /**< Start address of the MPU region. */
    uint32_t end;       /**< End address of the MPU region. */
    uint32_t config;    /**< MPU specific config data (permissions, subregions, etc). */
    UvisorBoxAcl acl;   /**< Original box ACL. */
} MpuRegion;

/* Region management */

/** Translate the start address, size and ACL into an MPU region.
 *
 * @param[out] region   MPU region provided by caller
 * @param start         start address of ACL region
 * @param size          size of the ACL region
 * @param acl           access rights of the ACL region
 * @returns             rounded size of the MPU region depending on ACL flags.
 * @retval 0            start, size or access rights are not consistent for this platform
 */
uint32_t vmpu_region_translate_acl(MpuRegion * const region, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec);

/** Bind an ACL to a box.
 *
 * @warning At the moment all ACLs for one box have to be added in one go.
 *          Do not interleave adding ACLs of different box ids!
 *
 * @param box_id        the box id to bind the ACL to
 * @param start         start address of ACL region
 * @param size          size of the ACL region
 * @param acl           access rights of the ACL region
 * @returns             rounded size of the MPU region depending on ACL flags.
 * @retval 0            start, size or access rights are not consistent for this platform
 */
uint32_t vmpu_region_add_static_acl(uint8_t box_id, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec);

/** Returns all MPU regions of a box as an array of `MpuRegion`s.
 *
 * @param box_id        the box id to get the ACLs for
 * @param[out] region   points to array of regions for this box, `NULL` if no regions availabl
 * @param[out] count    contains the number of regions in the array
 * @returns             `false` if box out-of-range, `true` otherwise
 */
bool vmpu_region_get_for_box(uint8_t box_id, const MpuRegion * * const region, uint32_t * const count);

/** Returns the MPU region of a box id for the specified address.
 *
 * @param box_id        the box id to look up the address in
 * @param address       the address to search in the the MPU regions
 * @returns             the MPU region that this address belongs to, `NULL` otherwise
 */
MpuRegion * vmpu_region_find_for_address(uint8_t box_id, uint32_t address);

/** Determine if the buffer is OK for a given box to access.
 *
 * @param box_id     the box id to look up the buffer in
 * @param addr       the buffer start address
 * @param size       the size of the buffer in bytes
 * @returns          true if the buffer is contained within the set of memory a
 *                   box could access, even if a fault and recovery is required
 *                   to access the buffer; false otherwise.
 * */
bool vmpu_buffer_access_is_ok(int box_id, const void * addr, size_t size);

/* Region management */

/* MPU access */

/** Initialize the MPU. */
void vmpu_mpu_init(void);

/** Write an ACL directly into an MPU region.
 * This is used to set ACLs that are persistent, like unlocking public Flash and insecure box SRAM.
 *
 * @warning These function may only be called after `vmpu_mpu_init()`, but before `vmpu_mpu_lock()`!
 *
 * @param index         MPU index to write this region to
 * @param start         start address of ACL region
 * @param size          size of the ACL region
 * @param acl           access rights for the ACL region
 * @param acl_hw_spec   hardware-specific access rights for the ACL region
 * @returns             rounded size of the MPU region depending on ACL flags.
 * @retval 0            index, start, size or access rights are not consistent for this platform
 */
uint32_t vmpu_mpu_set_static_acl(uint8_t index, uint32_t start, uint32_t size,
    UvisorBoxAcl acl, uint32_t acl_hw_spec);

/** Lock the MPU after setting global ACLs using `vmpu_mpu_set_static_acl`. */
void vmpu_mpu_lock(void);

/** Invalidate all configured MPU regions. */
void vmpu_mpu_invalidate(void);

/** Push a region into the MPU with the given priority.
 * A higher priority region replaces a lower priority region.
 * If no lower priority region can be found, the next viable region is replaced.
 * Regions with priority `255` are never replaced.
 * Regions with priority `0` are not allowed (they are upgraded to priority `1`).
 *
 * @param region    MPU region to enable
 * @param priority  region priority in the range of `1` to `254`.
 * @retval          `true` a viable MPU region was found immediately
 * @retval          `false` the search for a viable MPU region wrapped around.
 */
bool vmpu_mpu_push(const MpuRegion * const region, uint8_t priority);

/* MPU access */

#endif /* __VMPU_MPU_H__ */
