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

#define AIPSx_SLOT_SIZE 0x1000UL
#define AIPSx_SLOT_MAX 0xFE
#define AIPSx_DWORDS ((AIPSx_SLOT_MAX+31)/32)

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

    /* calculate resulting APIS slot and base */
    base = AIPS0_BASE | (((uint32_t)(aips_slot)) << 12);
    DPRINTF("\t\tAPIS slot[%02i] for base 0x%08X\n",
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
