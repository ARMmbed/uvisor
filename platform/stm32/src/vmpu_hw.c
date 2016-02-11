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
#include <uvisor.h>
#include "vmpu.h"
#include "debug.h"

#define VMPU_GRANULARITY 0x400
#define VMPU_ID_COUNT 0x100

/* The MPU regions for the SRAM depend on whether we use the CCM or the regular
 * SRAM. Not all the devices in the STM32 family have the CCM. */
#ifdef __STM32_HAS_CCM

#define SRAM_ORIGIN_NO_CCM     0x20000000
#define SRAM_LENGTH_MAX_NO_CCM 0x40000

static uint32_t g_vmpu_public_flash;

void vmpu_arch_init_hw(void)
{
    /* Determine public Flash. */
    g_vmpu_public_flash = vmpu_acl_static_region(
        0,
        (void *) FLASH_ORIGIN,
        ((uint32_t) __uvisor_config.secure_end) - FLASH_ORIGIN,
        UVISOR_TACLDEF_SECURE_CONST | UVISOR_TACL_EXECUTE
    );

    /* Enable box 0 SRAM.
     * Note: The region can be larger than the physical SRAM. */
    vmpu_acl_static_region(
        1,
        (void *) SRAM_ORIGIN_NO_CCM,
        SRAM_LENGTH_MAX_NO_CCM,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE
    );

    /* Check that the correct symbol has been defined in config.h.
     * Increase define if necessary. */
    if(ARMv7M_MPU_RESERVED_REGIONS != 2) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Please adjust ARMv7M_MPU_RESERVED_REGIONS to actual region count");
    }
}
#else /* __STM32_HAS_CCM */
#error "MPU configurations for STM32 devices without a CCM are not implemented yet."
#endif /* unsupported */
