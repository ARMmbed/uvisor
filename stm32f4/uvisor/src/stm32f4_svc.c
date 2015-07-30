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
    HALT_ERROR(NOT_IMPLEMENTED,
               "svc_write32 is not yet supported on this platform\n\r");
}
