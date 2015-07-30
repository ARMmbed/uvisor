/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include "svc.h"
#include "halt.h"

/* FIXME this function is temporary. Writes to an address should be checked
 *       against a box's ACLs */
void svc_write32(uint32_t *addr, uint32_t val)
{
    /* FIXME addresses for bitbanding are now hardcoded and will be replaced by
     * a system-wide API for bitband access */
    if((addr >= BITBAND_ADDRESS(&SIM->SCGC1, 0) &&
        addr <= BITBAND_ADDRESS(&SIM->SCGC7, 31))   ||
       (addr >= &SIM->SOPT1 &&
        addr <= &SIM->SOPT1 + sizeof(SIM->SOPT1)))
    {
        DPRINTF("Executed privileged access to 0x%08X\n\r", addr);
        *addr = val;
    }
    else
        HALT_ERROR(NOT_ALLOWED, "Bitband access currently only limited to\
                                 SIM->SCGCn registers\n\r");
}
