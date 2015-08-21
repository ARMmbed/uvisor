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
#include <debug.h>
#include <halt.h>
#include <memory_map.h>

void debug_mpu_config(void)
{
    uint32_t dregion, ctrl, size, rasr, ap, tex, srd;
    char dim[][3] = {"B ", "KB", "MB", "GB"};
    int i;

    dprintf("* MPU CONFIGURATION\n\r");

    /* CTRL */
    ctrl = MPU->CTRL;
    dprintf("\n\r");
    dprintf("  Background region %s\n\r", ctrl & MPU_CTRL_PRIVDEFENA_Msk ?
                                          "enabled" : "disabled");
    dprintf("  MPU %s @NMI, @HardFault\n\r", ctrl & MPU_CTRL_HFNMIENA_Msk ?
                                             "enabled" : "bypassed");
    dprintf("  MPU %s\n\r", ctrl & MPU_CTRL_PRIVDEFENA_Msk ?
                                  "enabled" : "disabled");
    dprintf("\n\r");

    /* information for each region (RBAR, RASR) */
    dregion = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    dprintf("  Region Start      Size  XN AP  TEX S C B SRD      Valid\n\r");
    for(i = 0; i < dregion; ++i)
    {
        /* select region */
        MPU->RNR = i;

        /* read RASR fields */
        rasr = MPU->RASR;
        ap   = (rasr & MPU_RASR_AP_Msk)  >> MPU_RASR_AP_Pos;
        tex  = (rasr & MPU_RASR_TEX_Msk) >> MPU_RASR_TEX_Pos;
        srd  = (rasr & MPU_RASR_SRD_Msk) >> MPU_RASR_SRD_Pos;

        /* size is used to compute the actual size in bytes, using the
         * conversion table provided in the ARMv7M manual */
        size = ((rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos) + 1;

        /* print information for region (fields are printed as bits) */
        dprintf("  %d      0x%08X %03d%s %d  %d%d%d %d%d%d %d %d %d ",
                i,
                MPU->RBAR & MPU_RBAR_ADDR_Msk,
                size > 4  ? 0x1 << (size % 10) : 0,
                dim[size / 10],
                (rasr & MPU_RASR_XN_Msk)  >> MPU_RASR_XN_Pos,
                (ap  & 0x1) >> 0x0, (ap  & 0x2) >> 0x1, (ap  & 0x4) >> 0x2,
                (tex & 0x1) >> 0x0, (tex & 0x2) >> 0x1, (tex & 0x4) >> 0x2,
                (rasr & MPU_RASR_S_Msk)   >> MPU_RASR_S_Pos,
                (rasr & MPU_RASR_C_Msk)   >> MPU_RASR_C_Pos,
                (rasr & MPU_RASR_B_Msk)   >> MPU_RASR_B_Pos,
                (rasr & MPU_RASR_SRD_Msk) >> MPU_RASR_B_Pos);
        dprintf("%d%d%d%d%d%d%d%d %d\n\r",
                (srd & 0x01) >> 0x0, (srd & 0x02) >> 0x1,
                (srd & 0x04) >> 0x2, (srd & 0x08) >> 0x3,
                (srd & 0x10) >> 0x4, (srd & 0x20) >> 0x5,
                (srd & 0x40) >> 0x6, (srd & 0x80) >> 0x7,
                rasr & MPU_RASR_ENABLE_Msk ? 1 : 0);
    }
    dprintf("\n\r");
}

void debug_mpu_fault(void)
{
}

void debug_map_addr_to_periph(uint32_t address)
{
}
