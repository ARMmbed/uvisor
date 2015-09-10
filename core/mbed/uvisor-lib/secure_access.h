/*
 * Copyright (c) 2015, ARM Limited, All Rights Reserved
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
#ifndef __SECURE_ACCESS_H__
#define __SECURE_ACCESS_H__

#include "uvisor-lib/vmpu_exports.h"

/* the switch statement will be optimised away since the compiler already knows
 * the sizeof(type) */
#define ADDRESS_WRITE(type, addr, val) \
    { \
        switch(sizeof(type)) \
        { \
            case 4: \
                uvisor_write32((volatile uint32_t *) (addr), (uint32_t) (val)); \
                break; \
            case 2: \
                uvisor_write16((volatile uint16_t *) (addr), (uint16_t) (val)); \
                break; \
            case 1: \
                uvisor_write8(( volatile uint8_t  *) (addr), (uint8_t ) (val)); \
                break; \
            default: \
                uvisor_error(USER_NOT_ALLOWED); \
                break; \
        } \
    }

/* the conditional statement will be optimised away since the compiler already
 * knows the sizeof(type) */
#define ADDRESS_READ(type, addr) \
    (sizeof(type) == 4 ? uvisor_read32((volatile uint32_t *) (addr)) : \
     sizeof(type) == 2 ? uvisor_read16((volatile uint16_t *) (addr)) : \
     sizeof(type) == 1 ? uvisor_read8(( volatile uint8_t  *) (addr)) : 0)

#define UNION_READ(type, addr, fieldU, fieldB) \
    ({ \
        type res; \
        res.fieldU = ADDRESS_READ(type, addr); \
        res.fieldB; \
    })

static inline __attribute__((always_inline)) void uvisor_write32(uint32_t volatile *addr, uint32_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint32_t r1 __asm__("r1") = val;
    __asm__ volatile(
        "str %[v], [%[a]]\n"
        UVISOR_NOP_GROUP
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) void uvisor_write16(uint16_t volatile *addr, uint16_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint16_t r1 __asm__("r1") = val;
    __asm__ volatile(
        "strh %[v], [%[a]]\n"
        UVISOR_NOP_GROUP
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) void uvisor_write8(uint8_t volatile *addr, uint8_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint8_t  r1 __asm__("r1") = val;
    __asm__ volatile(
        "strb %[v], [%[a]]\n"
        UVISOR_NOP_GROUP
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) uint32_t uvisor_read32(uint32_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint32_t res __asm__("r0");
    __asm__ volatile(
        "ldr %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return (uint32_t) res;
}

static inline __attribute__((always_inline)) uint16_t uvisor_read16(uint16_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint16_t res __asm__("r0");
    __asm__ volatile(
        "ldrh %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return res;
}

static inline __attribute__((always_inline)) uint8_t uvisor_read8(uint8_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint8_t  res __asm__("r0");
    __asm__ volatile(
        "ldrb %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return res;
}

#endif/*__SECURE_ACCESS_H__*/
