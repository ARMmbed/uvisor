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
#include <svc.h>
#include <vmpu.h>
#include "vmpu_freescale_k64_aips.h"

#define AIPSx_SLOT_SIZE    0x1000UL
#define AIPSx_SLOT_MAX     0xFE
#define AIPSx_DWORDS       ((AIPSx_SLOT_MAX+31)/32)
#define PACRx_DEFAULT_MASK 0x44444444UL

static uint32_t g_aipsx_box[UVISOR_MAX_BOXES][AIPSx_DWORDS];
static uint32_t g_aipsx_all[AIPSx_DWORDS];
static uint32_t g_aipsx_exc[AIPSx_DWORDS];

int vmpu_aips_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    int i, slot_count;
    uint8_t aips_slot;
    uint32_t base, t;

    if(    !((((uint32_t)start)>=AIPS0_BASE) &&
        (((uint32_t)start)<(AIPS0_BASE+(0xFEUL*(AIPSx_SLOT_SIZE))))) )
        return 0;

    aips_slot = (uint8_t)(((uint32_t)start) >> 12);
    if(aips_slot > AIPSx_SLOT_MAX)
    {
        DPRINTF("AIPS slot out of range (0xFF)");
        return -3;
    }

    /* calculate resulting AIPS slot and base */
    base = AIPS0_BASE | (((uint32_t)(aips_slot)) << 12);
    DPRINTF("\t\tAIPS slot[%02i] for base 0x%08X\n",
        aips_slot, base);

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


    /* initialize box[0] ACL's as background regions */
    if(box_id)
        memcpy(g_aipsx_box[box_id], g_aipsx_box[0], sizeof(g_aipsx_box[0]));

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
        g_aipsx_box[box_id][i] |= t;
        /* remember exclusive peripherals */
        if( (acl & UVISOR_TACL_SHARED) == 0 )
            g_aipsx_exc[i] |= t;

        /* process next slot */
        aips_slot++;
    }

    return 1;
}

void vmpu_aips_switch(uint8_t src_box, uint8_t dst_box)
{
    int i, j, k;
    uint32_t *psrc_acl, *pdst_acl;
    uint32_t src_acl, dst_acl, mod_acl;
    uint32_t pacr;
    volatile uint32_t *ppacr;

    /* beginning of AIPS0 */
    ppacr = &AIPS0->PACRA;
    /* initialize destination box pointer */
    psrc_acl = g_aipsx_box[src_box];
    pdst_acl = g_aipsx_box[dst_box];

    for(i=0; i<AIPSx_DWORDS; i++)
    {
        dst_acl = *pdst_acl++;
        src_acl = *psrc_acl++;

        /* skip reserved words */
        switch(i)
        {
            case 1:
            case 5:
                ppacr += 4;
                break;
            case 4:
                ppacr = &AIPS1->PACRA;
                break;
        }

        /* if src_box == dst_box, then initialize all fields */
        if(src_box == dst_box)
            mod_acl = 0xFFFFFFFF;
        else
        {
            mod_acl = src_acl ^ dst_acl;
            /* skip 4 PACRn altogether */
            if(!mod_acl)
            {
                ppacr += 4;
                continue;
            }
        }

        /* run through 4 PACRn's */
        for(j = 0; j < 4; j++)
        {
            /* modified byte ? */
            if((uint8_t)mod_acl)
            {
                pacr = 0;

                for(k = 0; k < 8; k++)
                {
                    pacr <<= 4;
                    pacr |= (dst_acl & 1) << 2;
                    dst_acl >>= 1;
                }

                pacr ^= PACRx_DEFAULT_MASK;
                *ppacr = pacr;
            }
            else
                dst_acl >>= 8;

            mod_acl >>= 8;
            ppacr++;
        }
    }
}
