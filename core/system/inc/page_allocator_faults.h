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

#endif /* __PAGE_ALLOCATOR_FAULTS_H__ */
