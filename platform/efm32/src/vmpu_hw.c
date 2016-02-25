/*
 * Copyright 2016 Silicon Laboratories, Inc.
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
#include "vmpu.h"
#include "debug.h"

#define VMPU_GRANULARITY 0x400
#define VMPU_ID_COUNT 0x100

#define SRAM_LENGTH_MAX 0x20000

static uint32_t g_vmpu_public_flash;

void vmpu_arch_init_hw(void)
{
    /* Determine public flash */
    g_vmpu_public_flash = vmpu_acl_static_region(
        0,
        (void*)FLASH_ORIGIN,
        ((uint32_t) __uvisor_config.secure_end) - FLASH_ORIGIN,
        UVISOR_TACLDEF_SECURE_CONST | UVISOR_TACL_EXECUTE
    );

    /* Enable box zero SRAM, region can be larger than physical SRAM */
    vmpu_acl_static_region(
        1,
        (void*)SRAM_ORIGIN,
        SRAM_LENGTH_MAX,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE
    );

    /* UVisor */
    vmpu_acl_static_region(
        2,
        (void*)SRAM_ORIGIN,
        ((uint32_t)__uvisor_config.bss_end)-SRAM_ORIGIN,
        UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE
    );

    /* Check that the correct symbol has been defined in config.h.
     * Increase define if necessary. */
    if(ARMv7M_MPU_RESERVED_REGIONS != 3) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Please adjust ARMv7M_MPU_RESERVED_REGIONS to actual region count");
    }
}
