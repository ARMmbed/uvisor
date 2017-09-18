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
#include "debug.h"
#include "halt.h"
#include "vmpu_kinetis_map.h"

static void debug_fault_mpu(void)
{
    uint32_t cesr = MPU->CESR;
    uint32_t sperr = cesr >> 27;
    uint32_t edr, ear, eacd;
    int s, r, i, found;

    if (sperr) {
        for (s = 4; s >= 0; s--) {
            if (sperr & (1 << s)) {
                edr = MPU->SP[4 - s].EDR;
                ear = MPU->SP[4 - s].EAR;
                eacd = edr >> 20;

                dprintf("* MPU FAULT:\n");
                dprintf("  Slave port:       %d\n", 4 - s);
                dprintf("  Address:          0x%08X\n", ear);
                dprintf("  Faulting regions: ");
                found = 0;
                for (r = 11; r >= 0; r--) {
                    if (eacd & (1 << r)) {
                        if (!found) {
                            dprintf("\n");
                            found = 1;
                        }
                        dprintf("    R%02d:", 11 - r);
                        for (i = 0; i < 4; i++) {
                            dprintf(" 0x%08X", MPU->WORD[11 - r][i]);
                        }
                        dprintf("\n");
                    }
                }
                if (!found) {
                    dprintf("[none]\n");
                }
                dprintf("  Master port:      %d\n", (edr >> 4) & 0xF);
                dprintf("  Error attribute:  %s %s (%s mode)\n",
                        edr & 0x2 ? "Data" : "Instruction",
                        edr & 0x1 ? "WRITE" : "READ",
                        edr & 0x4 ? "supervisor" : "user");
                break;
            }
        }
    } else {
        dprintf("* No MPU violation found\n");
    }
    dprintf("\n");
}

void debug_mpu_config(void)
{
    int i, j;

    dprintf("* MPU CONFIGURATION\n");

    /* CESR */
    dprintf("  CESR: 0x%08X\n", MPU->CESR);
    /* EAR, EDR */
    dprintf("       Slave 0    Slave 1    Slave 2    Slave 3    Slave 4\n");
    dprintf("  EAR:");
    for (i = 0; i < 5; i++) {
        dprintf(" 0x%08X", MPU->SP[i].EAR);
    }
    dprintf("\n  EDR:");
    for (i = 0; i < 5; i++) {
        dprintf(" 0x%08X", MPU->SP[i].EDR);
    }
    dprintf("\n");
    /* region descriptors */
    dprintf("       Start      End        Perm.      Valid\n");
    for (i = 0; i < 12; i++) {
        dprintf("  R%02d:", i);
        for (j = 0; j < 4; j++) {
            dprintf(" 0x%08X", MPU->WORD[i][j]);
        }
        dprintf("\n");
    }
    dprintf("\n");
    /* the alternate view is not printed */
}

void debug_fault_bus_hw(void)
{
    debug_fault_mpu();
}
