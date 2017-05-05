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
#include "svc.h"
#include "vmpu.h"
#include "vmpu_kinetis_aips.h"

#define AIPSx_DWORDS       ((AIPSx_SLOT_MAX+31)/32)
#define PACRx_DEFAULT_MASK 0x44444444UL

static uint32_t g_aipsx_box[UVISOR_MAX_BOXES][AIPSx_DWORDS];
static uint32_t g_aipsx_all[AIPSx_DWORDS];
static uint32_t g_aipsx_exc[AIPSx_DWORDS];

int vmpu_aips_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    int i, slot_count, box_count;
    uint8_t aips_slot;
    uint32_t base, t, address;

    if(    !((((uint32_t)start)>=AIPS0_BASE) &&
        (((uint32_t)start)<(AIPS0_BASE+(0xFEUL*(AIPSx_SLOT_SIZE))))) )
        return 0;

    if((acl & UVISOR_TACL_UWRITE)==0) {
        DPRINTF("read-only AIPS slots are currently not supported\n");
        return -2;
    }

    /* normalize address to AIPS0_BASE */
    address = ((uint32_t)start) - AIPS0_BASE;
    aips_slot = (uint8_t)(address >> 12);
    if(aips_slot > AIPSx_SLOT_MAX)
    {
        DPRINTF("AIPS slot out of range (0x%02X)\n", AIPSx_SLOT_MAX);
        return -3;
    }

    /* calculate resulting AIPS slot and base */
    base = AIPS0_BASE | (((uint32_t)(aips_slot)) << 12);

    /* check if ACL base is equal to slot base */
    if(base != (uint32_t)start)
    {
        DPRINTF("AIPS base (0x%08X) != ACL base (0x%08X)", base, start);
        return -4;
    }

    /* ensure that ACL size can be rounded up to slot size */
    if(((size % AIPSx_SLOT_SIZE) != 0) && ((acl & UVISOR_TACL_SIZE_ROUND_UP)==0))
    {
        DPRINTF("Use UVISOR_TACL_SIZE_ROUND_UP to round ACL size to AIPS slot size");
        return -5;
    }

    /* calculate slot count - round up */
    slot_count = (uint8_t)((base+size+4095) >> 12);
    if(slot_count>AIPSx_SLOT_MAX)
    {
        DPRINTF("AIPS slot size out of range (%i slots)", slot_count);
        return -6;
    }
    slot_count -= aips_slot;

    /* iterate through all slots */
    while(slot_count--)
    {
        i = aips_slot / 32;
        t = 1UL << ( aips_slot % 32);
        if( (g_aipsx_exc[i] & t) ||
            ((g_aipsx_all[i] & t) && ((acl & UVISOR_TACL_SHARED) == 0)) )
        {
            DPRINTF("AIPS slot %i already in use - redeclared at box %i",
                aips_slot, box_id);
            return -7;
        }

        /* ensure we have a list of all peripherals */
        g_aipsx_all[i] |= t;

        /* remember box-specific peripherals */
        if (box_id) {
            g_aipsx_box[box_id][i] |= t;
        }
        else {
            /* box 0 ACLs are applied to all boxes */
            for (box_count = 0; box_count < g_vmpu_box_count; box_count++) {
                g_aipsx_box[box_count][i] |= t;
            }
        }

        /* remember exclusive peripherals */
        if( (acl & UVISOR_TACL_SHARED) == 0 )
            g_aipsx_exc[i] |= t;

        /* process next slot */
        aips_slot++;
    }

    return 1;
}

uint32_t vmpu_fault_find_acl_aips(uint8_t active_box, uint32_t fault_addr, uint32_t size)
{
    uint32_t aips_slot, base;
    uint32_t *pdst_acl;

    /* normalize address to AIPS0_BASE */
    aips_slot = (fault_addr - AIPS0_BASE) >> 12;
    if(aips_slot > AIPSx_SLOT_MAX)
    {
        DPRINTF("AIPS slot out of range (0x%02X)\n", AIPSx_SLOT_MAX);
        return 0;
    }

    /* calculate resulting AIPS slot and base */
    base = AIPS0_BASE | (aips_slot << 12);
    /* ensure that data fits in selected region */
    if((size>sizeof(uint32_t)) || ((fault_addr+size)>(base+AIPSx_SLOT_SIZE)))
        return 0;

    /* get active box */
    pdst_acl = g_aipsx_box[active_box];

    /* check peripheral ACL */
    if(pdst_acl[aips_slot / 32] & (1UL << (aips_slot % 32)))
        return UVISOR_TACLDEF_PERIPH;
    else
        return 0;
}

void vmpu_aips_switch(uint8_t src_box, uint8_t dst_box)
{
    /* Beginning of AIPS0 */
    uint32_t volatile * ppacr = &AIPS0->PACRA;

    /* Initialize the destination box pointer. */
    uint32_t * psrc_acl = g_aipsx_box[src_box];
    uint32_t * pdst_acl = g_aipsx_box[dst_box];

    for (size_t i = 0; i < AIPSx_DWORDS; i++) {
        uint32_t dst_acl = *pdst_acl++;
        uint32_t src_acl = *psrc_acl++;

        /* Skip the reserved words. */
        switch (i) {
        case 1:
        case 5:
            ppacr += 4;
            break;
        case 4:
            ppacr = &AIPS1->PACRA;
            break;
        }

        /* If the source and destination box are the same, initialize all
         * fields. */
        uint32_t mod_acl = 0;
        if (src_box == dst_box) {
            mod_acl = 0xFFFFFFFF;
        } else {
            mod_acl = src_acl ^ dst_acl;

            /* Skip 4 PACRn altogether if there's nothing to update. */
            if (!mod_acl) {
                ppacr += 4;
                continue;
            }
        }

        /* Apply the changes to 4 consecurive PACRn's. */
        for (size_t j = 0; j < 4; j++) {
            /* Skip this byte if there is nothing to update. */
            if (!((uint8_t) mod_acl)) {
                dst_acl >>= 8;
            } else {
                uint32_t pacr = 0;
                for (size_t k = 0; k < 8; k++) {
                    pacr <<= 4;
                    pacr |= (dst_acl & 1) << 2;
                    dst_acl >>= 1;
                }
                pacr ^= PACRx_DEFAULT_MASK;
                *ppacr = pacr;
            }
            mod_acl >>= 8;
            ppacr++;
        }
    }
}
