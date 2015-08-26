/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
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
#ifndef __VMPU_UNPRIV_ACCESS_H__
#define __VMPU_UNPRIV_ACCESS_H__

static inline __attribute__((always_inline)) void vmpu_unpriv_uint16_write(uint32_t addr, uint16_t data)
{
    asm volatile(
        "strt %[data], [%[addr]]"
    :: [addr] "r" (addr), [data] "r" (data));
}

static inline __attribute__((always_inline)) void vmpu_unpriv_uint32_write(uint32_t addr, uint32_t data)
{
    asm volatile(
        "strt %[data], [%[addr]]"
    :: [addr] "r" (addr), [data] "r" (data));
}

static inline __attribute__((always_inline)) uint16_t vmpu_unpriv_uint16_read(uint32_t addr)
{
    uint16_t res;
    asm volatile(
        "ldrt %[res], [%[addr]]"
    : [res] "=r" (res) : [addr] "r" (addr));
    return res;
}

static inline __attribute__((always_inline)) uint32_t vmpu_unpriv_uint32_read(uint32_t addr)
{
    uint32_t res;
    asm volatile(
        "ldrt %[res], [%[addr]]"
    : [res] "=r" (res) : [addr] "r" (addr));
    return res;
}

#endif/*__VMPU_UNPRIV_ACCESS_H__*/
