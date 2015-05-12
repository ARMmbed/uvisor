/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/

#include <uvisor.h>
#include "svc.h"
#include "vmpu.h"
#include "vmpu_aips.h"

#define AIPSx_SLOT_SIZE    0x1000UL
#define AIPSx_SLOT_MAX     0xFE
#define AIPSx_DWORDS       ((AIPSx_SLOT_MAX+31)/32)
#define PACRx_DEFAULT_MASK 0x44444444UL

static uint32_t g_aipsx_box[UVISOR_MAX_BOXES][AIPSx_DWORDS];
static uint32_t g_aipsx_all[AIPSx_DWORDS];
static uint32_t g_aipsx_exc[AIPSx_DWORDS];

int vmpu_add_aips(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
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

    /* calculate slot count */
    slot_count = (uint8_t)((base+size) >> 12);
    if(slot_count>AIPSx_SLOT_MAX)
    {
        DPRINTF("AIPS slot size out of range (%i slots)", slot_count);
        return -5;
    }
    slot_count -= aips_slot;

    /* ensure that ACL size can be rounded up to slot size */
    if(((size % AIPSx_SLOT_SIZE) != 0) && ((acl & UVISOR_TACL_SIZE_ROUND_UP)==0))
    {
        DPRINTF("Use UVISOR_TACL_SIZE_ROUND_UP to round ACL size to AIPS slot size");
        return -6;
    }

    i = aips_slot / 32;
    t = 1 << ( aips_slot % 32);
    if( (g_aipsx_exc[i] & t) ||
        ((g_aipsx_all[i] & t) && ((acl & UVISOR_TACL_SHARED) == 0)) )
    {
        DPRINTF("AIPS slot %i already in use - redeclared at box %i",
            aips_slot, box_id);
        return -7;
    }

    /* initialize box[0] ACL's as background regions */
    if(box_id)
        memcpy(g_aipsx_box[box_id], g_aipsx_box[0], sizeof(g_aipsx_box[0]));

    /* ensure we have a list of all peripherals */
    g_aipsx_all[i] |= t;
    /* remember box-specific peripherals */
    g_aipsx_box[box_id][i] |= t;
    /* remember exclusive peripherals */
    if( (acl & UVISOR_TACL_SHARED) == 0 )
        g_aipsx_exc[i] |= t;

    return 1;
}

void vmpu_aips_switch(uint8_t src_box, uint8_t dst_box)
{
    int i, j, k;
    uint8_t src_acl, dst_acl;
    uint32_t src_acl_w, dst_acl_w;
    uint32_t pacr, *pacr_ptr;

    /* initialized once - t will be renewed through shifting at each iteration */
    pacr = 0;

    /* beginning of AIPS0 */
    pacr_ptr = (uint32_t *) &AIPS0->PACRA;

    /* parsing words in g_aipsx_box */
    for(i = 0; i < AIPSx_DWORDS; i++)
    {
        /* if src and dst box are the same, the ACLs are applied without
         * optimizations; this is used for example to initialize box 0 ACls */
        if(src_box == dst_box)
            src_acl_w = 0;
        else
            src_acl_w = g_aipsx_box[src_box][i];
        dst_acl_w = g_aipsx_box[dst_box][i];

        /* detect changes in ACLs */
        if(src_acl_w != dst_acl_w)
        {
            /* parsing single PACRn registers (each configures 8 slots) */
            for(j = 0; j < 4; j++)
            {
                src_acl = (uint8_t) src_acl_w;
                dst_acl = (uint8_t) dst_acl_w;

                /* detect changes in ACLs */
                if(src_acl != dst_acl)
                {
                    /* turn 8bit flags in 32bit PACRn register */
                    for(k = 0; k < 8; k++)
                    {
                        /* shift previously parsed slots */
                        pacr <<= 4;
                        /* write SP0 field in PACRn for current slot */
                        pacr |= (dst_acl & 1) << 2;
                        /* shift to next slot */
                        dst_acl >>= 1;
                        /* endiannesses for pacr and dst_acl are inverted! */
                    }
                    /* write masked PACRn to AIPSx->PACRn */
                    *(pacr_ptr) = pacr ^ PACRx_DEFAULT_MASK;
                }
                /* go to next PACRn */
                pacr_ptr++;
                src_acl_w >>= 8;
                dst_acl_w >>= 8;
            }
        }
        else
        {
            /* skip 4 PACRn altogether */
            pacr_ptr += 4;
        }

        /* this is needed due to the AIPSx->PACRn memory map in Freescale K20
         * sub-family microcontrollers */
        if(i == 0 || i == 4)
            pacr_ptr += 4;
        else if(i == 3)
            pacr_ptr = (uint32_t *) &AIPS1->PACRA;
    }
}
