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
#include <uvisor.h>
#include <vmpu.h>
#include <debug.h>

#define VMPU_GRANULARITY 0x400
#define VMPU_ID_COUNT 0x100

static uint32_t g_vmpu_public_flash;

void vmpu_arch_init_hw(void)
{
    /* determine public flash */
    g_vmpu_public_flash = vmpu_acl_static_region(
        0,
        (void*)FLASH_ORIGIN,
        ((uint32_t)__uvisor_config.secure_end)-FLASH_ORIGIN,
        UVISOR_TACLDEF_SECURE_CONST | UVISOR_TACL_EXECUTE
    );

    /* enable box zero SRAM, region can be larger than physical SRAM */
    vmpu_acl_static_region(
        1,
        (void*)0x20000000,
        0x40000,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE
    );

    /* sanity check, increase define if necessary */
    if(ARMv7M_MPU_RESERVED_REGIONS != 2)
        HALT_ERROR(SANITY_CHECK_FAILED,
            "please adjust ARMv7M_MPU_RESERVED_REGIONS to actual region count");
}
