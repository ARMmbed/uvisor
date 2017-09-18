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
#include "debug.h"

static void debug_fault_mpu(void)
{
}

static void debug_fault_sau(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

void debug_mpu_config(void)
{
    /* On ARMv8-M we have 2 MPUs (S, NS). */
    MPU_Type * mpu_bases[2] = {MPU, MPU_NS};
    int mpu = 0;
    for (; mpu < 2; ++mpu) {
        dprintf("* MPU CONFIGURATION (%s)\n", (mpu == 0) ? "S" : "NS");
        dprintf("\n");

        /* MPU_TYPE register */
        /* Note: On ARMv8-M the "SEPARATE" bit is always 0, so it's omitted. */
        uint32_t type = mpu_bases[mpu]->TYPE;
        int nregions = (type & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
        dprintf("  --> %d regions available.\n", nregions);

        /* MPU_CTRL register */
        uint32_t ctrl = mpu_bases[mpu]->CTRL;
        dprintf("  --> The MPU is %s.\n", (ctrl & MPU_CTRL_ENABLE_Msk) ? "enabled" : "disabled");
        dprintf("  --> The MPU is %s for HardFault/NMI exceptions.\n",
                (ctrl & MPU_CTRL_HFNMIENA_Msk) ? "enabled" : "disabled");
        dprintf("  --> By default %s code can execute from the system address map.\n",
                (ctrl & MPU_CTRL_PRIVDEFENA_Msk) ? "only privileged" : "all");

        /* Regions dump. */
        dprintf("\n");
        dprintf("  Region  Base        Limit       Shareable  Permissions      Attr  En\n");
        int region = 0;
        for (; region < nregions; ++region) {
            /* Select the region. */
            mpu_bases[mpu]->RNR = region;
            uint32_t rbar = mpu_bases[mpu]->RBAR;
            uint32_t rlar = mpu_bases[mpu]->RLAR;

            /* Region boundaries */
            uint32_t base = rbar & MPU_RBAR_ADDR_Msk;
            uint32_t limit = (rlar & MPU_RLAR_LIMIT_Msk) | 0x1F;

            /* Region shareability */
            uint32_t sh = (rbar & MPU_RBAR_SH_Msk) >> MPU_RBAR_SH_Pos;
            char sh_strings[4][6] = {
                "No   ",
                "[NA] ",
                "Outer",
                "Inner"
            };

            /* Region permissions */
            uint32_t ap = (rbar & MPU_RBAR_AP_Msk) >> MPU_RBAR_AP_Pos;
            char ap_strings_p[4][3] = {
                "RW",
                "RW",
                "R-",
                "R-"
            };
            char ap_strings_np[4][3] = {
                "--",
                "RW",
                "--",
                "R-"
            };
            bool xn = rbar & MPU_RBAR_XN_Msk;

            dprintf("  %03d     0x%08X  0x%08X  %s      P: %s%s, NP: %s%s  %02d    %s\n",
                    region,
                    base, limit,
                    sh_strings[sh],
                    ap_strings_p[ap], xn ? "-" : "X", ap_strings_np[ap], xn ? "-" : "X",
                    (rlar & MPU_RLAR_AttrIndx_Msk) >> MPU_RLAR_AttrIndx_Pos,
                    (rlar & MPU_RLAR_EN_Msk) ? "Y" : "N");
        }

        dprintf("\n");
    }
}

void debug_sau_config(void)
{
    dprintf("* SAU CONFIGURATION\n");
    dprintf("\n");

    /* SAU_CTRL register */
    uint32_t ctrl = SAU->CTRL;
    dprintf("  --> Memory is marked by default as %s.\n", (ctrl & SAU_CTRL_ALLNS_Msk) ? "NS" : "S, not NSC");
    dprintf("  --> The SAU is %sabled.\n", (ctrl & SAU_CTRL_ENABLE_Msk) ? "en" : "dis");

    /* SAU_TYPE register */
    uint32_t type = SAU->TYPE;
    int nregions = (type & SAU_TYPE_SREGION_Msk) >> SAU_TYPE_SREGION_Pos;
    dprintf("  --> %d regions available.\n", nregions);

    /* Regions dump. */
    dprintf("\n");
    dprintf("  Region  Base        Limit       NSC  En\n");
    int region = 0;
    for (; region < nregions; ++region) {
        /* Select the region. */
        SAU->RNR = region;
        uint32_t rbar = SAU->RBAR;
        uint32_t rlar = SAU->RLAR;

        dprintf("  %03d     0x%08X  0x%08X  %s    %s\n",
            region,
            rbar & SAU_RBAR_BADDR_Msk,
            (rlar & SAU_RLAR_LADDR_Msk) | 0x1F,
            (rlar & SAU_RLAR_NSC_Msk) ? "Y" : "N",
            (rlar & SAU_RLAR_ENABLE_Msk) ? "Y" : "N");
    }

    dprintf("\n");
}

void debug_fault_memmanage_hw(void)
{
    debug_fault_mpu();
}

void debug_fault_secure_hw(void)
{
    debug_fault_sau();
}
