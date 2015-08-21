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
#include <vmpu_freescale_k64_map.h>

static uint32_t debug_aips_slot_from_addr(uint32_t aips_addr)
{
    return ((aips_addr & 0xFFF000) >> 12);
}

static uint32_t debug_aips_bitn_from_alias(uint32_t alias, uint32_t aips_addr)
{
    uint32_t bitn;

    bitn = ((alias - MEMORY_MAP_BITBANDING_START) -
           ((aips_addr - MEMORY_MAP_PERIPH_START) << 5)) >> 2;

    return bitn;
}

static uint32_t debug_aips_addr_from_alias(uint32_t alias)
{
    uint32_t aips_addr;

    aips_addr  = ((((alias - MEMORY_MAP_BITBANDING_START) >> 2) & ~0x1F) >> 3);
    aips_addr += MEMORY_MAP_PERIPH_START;

    return aips_addr;
}

void debug_fault_bus(void)
{
    dprintf("* CFSR : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* BFAR : 0x%08X\n\r\n\r", SCB->BFAR);
    debug_mpu_fault();
    debug_map_addr_to_periph(SCB->BFAR);
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

void debug_mpu_fault(void)
{
    uint32_t cesr = MPU->CESR;
    uint32_t sperr = cesr >> 27;
    uint32_t edr, ear, eacd;
    int s, r, i, found;

    if(sperr)
    {
        for(s = 4; s >= 0; s--)
        {
            if(sperr & (1 << s))
            {
                edr = MPU->SP[4 - s].EDR;
                ear = MPU->SP[4 - s].EAR;
                eacd = edr >> 20;

                dprintf("* MPU FAULT:\n\r");
                dprintf("  Slave port:       %d\n\r", 4 - s);
                dprintf("  Address:          0x%08X\n\r", ear);
                dprintf("  Faulting regions: ");
                found = 0;
                for(r = 11; r >= 0; r--)
                {
                    if(eacd & (1 << r))
                    {
                        if(!found)
                        {
                            dprintf("\n\r");
                            found = 1;
                        }
                        dprintf("    R%02d:", 11 - r);
                        for (i = 0; i < 4; i++) {
                            dprintf(" 0x%08X", MPU->WORD[11 - r][i]);
                        }
                        dprintf("\n\r");
                    }
                }
                if(!found)
                    dprintf("[none]\n\r");
                dprintf("  Master port:      %d\n\r", (edr >> 4) & 0xF);
                dprintf("  Error attribute:  %s %s (%s mode)\n\r",
                        edr & 0x2 ? "Data" : "Instruction",
                        edr & 0x1 ? "WRITE" : "READ",
                        edr & 0x4 ? "supervisor" : "user");
                break;
            }
        }
    }
    else
        dprintf("* No MPU violation found\n\r");
    dprintf("\n\r");
}

void debug_map_addr_to_periph(uint32_t address)
{
    uint32_t aips_addr;
    const MemMap *map;

    dprintf("* MEMORY MAP\n");
    dprintf("  Address:           0x%08X\n", address);

    /* find system memory region */
    if((map = memory_map_name(address)) != NULL)
    {
        dprintf("  Region/Peripheral: %s\n", map->name);
        dprintf("    Base address:    0x%08X\n", map->base);
        dprintf("    End address:     0x%08X\n", map->end);

        if(address >= MEMORY_MAP_PERIPH_START &&
           address <= MEMORY_MAP_PERIPH_END)
        {
            dprintf("    AIPS slot:       %d\n",
                    debug_aips_slot_from_addr(map->base));
        }
        else if(address >= MEMORY_MAP_BITBANDING_START &&
                address <= MEMORY_MAP_BITBANDING_END)
        {
            dprintf("    Before bitband:  0x%08X\n",
                    (aips_addr = debug_aips_addr_from_alias(address)));
            map = memory_map_name(aips_addr);
            dprintf("    Alias:           %s\n", map->name);
            dprintf("      Base address:  0x%08X\n", map->base);
            dprintf("      End address:   0x%08X\n", map->end);
            dprintf("    Accessed bit:    %d\n",
                     debug_aips_bitn_from_alias(address, aips_addr));
            dprintf("    AIPS slot:       %d\n",
                     debug_aips_slot_from_addr(aips_addr));
        }

        dprintf("\n");
    }
}
