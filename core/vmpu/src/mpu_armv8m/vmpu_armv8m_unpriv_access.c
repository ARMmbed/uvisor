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
#include <uvisor.h>
#include "context.h"
#include "debug.h"
#include "vmpu.h"
#include "vmpu_unpriv_access.h"
#include <stdbool.h>

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
        /* Both memory locations have the same non-secure SAU region and therefore same properties.
         * No Secure SAU region can be inbetween due to SAU region overlap rules. */
        response_lower &= response_upper;
    }

    return response_lower & (TT_RESP_NSRW_Msk | TT_RESP_NSR_Msk | TT_RESP_RW_Msk | TT_RESP_R_Msk |
                             TT_RESP_S_Msk | TT_RESP_SRVALID_Msk | TT_RESP_SREGION_Msk);
}

extern int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status);

uint32_t vmpu_unpriv_access(uint32_t addr, uint32_t size, uint32_t data)
{
    unsigned int tries = 0;
    while(1) {
        if ((vmpu_unpriv_test_range(addr, UVISOR_UNPRIV_ACCESS_SIZE(size)) & (TT_RESP_NSRW_Msk | TT_RESP_SRVALID_Msk)) == (TT_RESP_NSRW_Msk | TT_RESP_SRVALID_Msk)) {
            switch(size) {
                case UVISOR_UNPRIV_ACCESS_READ(1):
                    return *((uint8_t *) addr);
                case UVISOR_UNPRIV_ACCESS_READ(2):
                    return *((uint16_t *) addr);
                case UVISOR_UNPRIV_ACCESS_READ(4):
                    return *((uint32_t *) addr);
                case UVISOR_UNPRIV_ACCESS_WRITE(1):
                    *((uint8_t *) addr) = (uint8_t) data;
                    return 0;
                case UVISOR_UNPRIV_ACCESS_WRITE(2):
                    *((uint16_t *) addr) = (uint16_t) data;
                    return 0;
                case UVISOR_UNPRIV_ACCESS_WRITE(4):
                    *((uint32_t *) addr) = data;
                    return 0;
                default:
                    break;
            }
            break;
        }
        if (++tries > 1 || !vmpu_fault_recovery_mpu(0, 0, addr, 0)) {
            break;
        }
    }
    HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
    return 0;
}
