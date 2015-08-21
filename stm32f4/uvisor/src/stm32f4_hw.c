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

static uint32_t g_peripheral_common[VMPU_ID_COUNT/32];
static uint32_t g_peripheral_acl[UVISOR_MAX_BOXES][VMPU_ID_COUNT/32];
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
}

int vmpu_addr2pid(uint32_t addr)
{
    uint8_t block = (uint8_t)(addr>>16);
    uint16_t block_low = (uint16_t)addr;

    switch((uint8_t)(addr>>24))
    {
        /* APB1, APB2, AHB1 */
        case 0x40:
            /* detect peripheral region ID 0x00-0xBF */
            if(block<=0x02)
                return (addr & 0x3FFFFUL)/VMPU_GRANULARITY;

            /* USB OTG HS */
            if((block>0x04) && (block<=0x07))
                return 0xC0;

            /* bail out if no match */
            break;

        /* FSMC or FMC */
        case 0xA0:
            return ((!block) && (block_low <= 0xFFF)) ? 0xC1 : -1;

        /* AHB2 */
        case 0x50:
            switch(block)
            {
                /* USB OTG FS */
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                    return 0xC2;

                /* DMCI */
                case 0x05:
                    return (block_low <= 0x3FF) ? 0xC3 : -1;

                /* crypto peripheral region ID 0xC4-0xC6 */
                case 0x06:
                    return (block_low <= 0xBFF) ?
                        0xC4+(block_low/VMPU_GRANULARITY) : -1;
            }
    }

    /* default is invalid peripheral */
    return -1;
}

void vmpu_add_peripheral_map(uint8_t box_id, uint32_t addr, uint32_t length, uint8_t shared)
{
    int pid, index;
    uint32_t mask;

    /* scan through whole peripheral length, mark all */
    while(1)
    {
        /* determine peripheral ID for given address */
        pid = vmpu_addr2pid(addr);
        if((pid<0) || (pid>=VMPU_ID_COUNT))
                HALT_ERROR(SANITY_CHECK_FAILED,
                    "pid invalid (%i) for address 0x%08X\n\r",
                    pid, addr
                );

        /* calculate table index and bitmask */
        index = pid / 32;
        mask = 1UL << (pid%32);

        /* ensure that boxes don't declare regions that exist in the
         * main box already */
        if((g_peripheral_common[index] & mask)==0)
        {
            /* mark regions */
            g_peripheral_common[index] |= mask;
            g_peripheral_acl[box_id][index] |= mask;
        }

        if(length<=VMPU_GRANULARITY)
            break;

        addr+=VMPU_GRANULARITY;
        length-=VMPU_GRANULARITY;
    }
}
