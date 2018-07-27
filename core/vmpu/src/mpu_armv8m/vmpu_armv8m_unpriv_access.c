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
#include <stdbool.h>
#include "context.h"
#include "halt.h"
#include "vmpu_mpu.h"
#include "vmpu_unpriv_access.h"

uint32_t vmpu_unpriv_access(uint32_t addr, uint32_t size, uint32_t data)
{
    // This operation could be slow since we are scanning all the regions defined for the box
    // an implementation using the TTA command is faster but causes misses since it works with the MPU
    if (vmpu_buffer_access_is_ok(g_active_box, (const void *) addr, UVISOR_UNPRIV_ACCESS_SIZE(size))){
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
    }
    HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
    return 0;
}
