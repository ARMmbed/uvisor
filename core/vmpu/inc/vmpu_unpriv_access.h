/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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

#include "api/inc/uvisor_exports.h"
#include "halt.h"
#include <stdint.h>

#if defined(ARCH_MPU_ARMv8M)

#define TT_RESP_IREGION_Pos     24U
#define TT_RESP_IREGION_Msk     (0xFFUL << TT_RESP_IREGION_Pos)

#define TT_RESP_IRVALID_Pos     23U
#define TT_RESP_IRVALID_Msk     (1UL << TT_RESP_IRVALID_Pos)

#define TT_RESP_S_Pos           22U
#define TT_RESP_S_Msk           (1UL << TT_RESP_S_Pos)

#define TT_RESP_NSRW_Pos        21U
#define TT_RESP_NSRW_Msk        (1UL << TT_RESP_NSRW_Pos)

#define TT_RESP_NSR_Pos         20U
#define TT_RESP_NSR_Msk         (1UL << TT_RESP_NSR_Pos)

#define TT_RESP_RW_Pos          19U
#define TT_RESP_RW_Msk          (1UL << TT_RESP_RW_Pos)

#define TT_RESP_R_Pos           18U
#define TT_RESP_R_Msk           (1UL << TT_RESP_R_Pos)

#define TT_RESP_SRVALID_Pos     17U
#define TT_RESP_SRVALID_Msk     (1UL << TT_RESP_SRVALID_Pos)

#define TT_RESP_MRVALID_Pos     16U
#define TT_RESP_MRVALID_Msk     (1UL << TT_RESP_MRVALID_Pos)

#define TT_RESP_SREGION_Pos     8U
#define TT_RESP_SREGION_Msk     (0xFFUL << TT_RESP_SREGION_Pos)

#define TT_RESP_MREGION_Pos     0U
#define TT_RESP_MREGION_Msk     (0xFFUL << TT_RESP_MREGION_Pos)


static uint32_t vmpu_unpriv_test_range(uint32_t addr, uint32_t size)
{
    if (!size) size = 1;
    uint32_t response_lower, response_upper;
    uint32_t test_addr_lower = addr & ~31UL;
    uint32_t test_addr_upper = (addr + size - 1) & ~31UL;

    /* Test lower address. */
    asm volatile (
        "tta %[response], %[addr]"
        : [response] "=r" (response_lower)
        : [addr] "r" (test_addr_lower)
    );
    if (test_addr_lower != test_addr_upper) {
        /* Test upper address. */
        asm volatile (
            "tta %[response], %[addr]"
            : [response] "=r" (response_upper)
            : [addr] "r" (test_addr_upper)
        );
        /* If lower and upper do not have the same S|SRVALID|SREGION, then it's definitely not the same region. */
        if (((response_lower ^ response_upper) & (TT_RESP_S_Msk | TT_RESP_SRVALID_Msk | TT_RESP_SREGION_Msk))) {
            /* Upper memory region has different SAU region than lower memory region! */
            return 0;
        }
        /* Reduce response to one bit field. */
        response_lower &= response_upper;
    }
    /* Both memory locations have the same SAU region and SAU valid bit. */
    if ((response_lower & (TT_RESP_S_Msk | TT_RESP_SRVALID_Msk)) == TT_RESP_SRVALID_Msk) {
        /* Both memory locations have the same non-secure SAU region and therefore same properties.
         * No Secure SAU region can be inbetween due to SAU region overlap rules. */
        return response_lower & (TT_RESP_NSRW_Msk |
                                 TT_RESP_NSR_Msk |
                                 TT_RESP_RW_Msk |
                                 TT_RESP_R_Msk |
                                 TT_RESP_SRVALID_Msk |
                                 TT_RESP_SREGION_Msk);
    }

    return 0;
}

#define UVISOR_UNPRIV_FORCEINLINE inline
#else
#define UVISOR_UNPRIV_FORCEINLINE UVISOR_FORCEINLINE
#endif

/** Write an 8-bit value to an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 *
 * @param addr[in]  Address to write to.
 * @param data[in]  8-bit data to write.
 */
static UVISOR_UNPRIV_FORCEINLINE void vmpu_unpriv_uint8_write(uint32_t addr, uint8_t data)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint8_t)) & TT_RESP_NSRW_Msk) {
        *((uint8_t *) addr) = data;
    }
    else {
        HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
        halt_user_error(PERMISSION_DENIED);
    }
#else
    asm volatile (
        "strbt %[data], [%[addr]]\n"
        :: [addr] "r" (addr), [data] "r" (data)
    );
#endif
}

/** Write a 16-bit value to an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 * otherwise HardFault).
 *
 * @param addr[in]  Address to write to.
 * @param data[in]  16-bit data to write.
 */
static UVISOR_UNPRIV_FORCEINLINE void vmpu_unpriv_uint16_write(uint32_t addr, uint16_t data)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint16_t)) & TT_RESP_NSRW_Msk) {
        *((uint16_t *) addr) = data;
    }
    else {
        HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
        halt_user_error(PERMISSION_DENIED);
    }
#else
    asm volatile (
        "strht %[data], [%[addr]]\n"
        :: [addr] "r" (addr), [data] "r" (data)
    );
#endif
}

/** Write a 32-bit value to an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 * otherwise HardFault).
 *
 * @param addr[in]  Address to write to.
 * @param data[in]  32-bit data to write.
 */
static UVISOR_UNPRIV_FORCEINLINE void vmpu_unpriv_uint32_write(uint32_t addr, uint32_t data)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint32_t)) & TT_RESP_NSRW_Msk) {
        *((uint32_t *) addr) = data;
    }
    else {
        HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
        halt_user_error(PERMISSION_DENIED);
    }
#else
    asm volatile (
        "strt %[data], [%[addr]]\n"
        :: [addr] "r" (addr), [data] "r" (data)
    );
#endif
}

/** Read an 8-bit value from an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 * otherwise HardFault).
 *
 * @param addr[in]  Address to read from.
 * @returns the 8-bit value read from the address.
 */
static UVISOR_UNPRIV_FORCEINLINE uint8_t vmpu_unpriv_uint8_read(uint32_t addr)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint8_t)) & TT_RESP_NSR_Msk) {
        return *((uint8_t *) addr);
    }
    HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
    halt_user_error(PERMISSION_DENIED);
    return 0;
#else
    uint8_t res;
    asm volatile (
        "ldrbt %[res], [%[addr]]\n"
        : [res] "=r" (res)
        : [addr] "r" (addr)
    );
    return res;
#endif
}

/** Read a 16-bit value from an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 * otherwise HardFault).
 *
 * @param addr[in]  Address to read from.
 * @returns the 16-bit value read from the address.
 */
static UVISOR_UNPRIV_FORCEINLINE uint16_t vmpu_unpriv_uint16_read(uint32_t addr)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint16_t)) & TT_RESP_NSR_Msk) {
        return *((uint16_t *) addr);
    }
    HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
    halt_user_error(PERMISSION_DENIED);
    return 0;
#else
    uint16_t res;
    asm volatile (
        "ldrht %[res], [%[addr]]\n"
        : [res] "=r" (res)
        : [addr] "r" (addr)
    );
    return res;
#endif
}

/** Read a 32-bit value from an address as if in unprivileged mode.
 *
 * This function can be used even if the MPU is disabled, but the architectural
 * restrictions on privileged-only registers still apply.
 *
 * @warning: Upon failure this function triggers a fault (MemManage if enabled,
 * otherwise HardFault).
 *
 * @param addr[in]  Address to read from.
 * @returns the 32-bit value read from the address.
 */
static UVISOR_UNPRIV_FORCEINLINE uint32_t vmpu_unpriv_uint32_read(uint32_t addr)
{
#if defined(ARCH_MPU_ARMv8M)
    if (vmpu_unpriv_test_range(addr, sizeof(uint32_t)) & TT_RESP_NSR_Msk) {
        return *((uint32_t *) addr);
    }
    HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
    halt_user_error(PERMISSION_DENIED);
    return 0;
#else
    uint32_t res;
    asm volatile (
        "ldrt %[res], [%[addr]]\n"
        : [res] "=r" (res)
        : [addr] "r" (addr)
    );
    return res;
#endif
}

#endif /* __VMPU_UNPRIV_ACCESS_H__ */
