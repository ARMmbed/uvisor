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
#include "mbed/mbed.h"
#include "uvisor-lib/uvisor-lib.h"

void uvisor_write_bitband(uint32_t volatile *addr, uint32_t val)
{
    if (__uvisor_mode == 0)
    {
        *addr = val;
    }
    else
    {
        register uint32_t __r0 __asm__("r0") = (uint32_t) addr;
        register uint32_t __r1 __asm__("r1") = (uint32_t) val;
        __asm__ volatile(
            "svc  %[svc_id]\n"
            :
            :          "r" (__r0),
                       "r" (__r1),
              [svc_id] "i" (UVISOR_SVC_ID_WRITE_BITBAND)
        );
    }
}
