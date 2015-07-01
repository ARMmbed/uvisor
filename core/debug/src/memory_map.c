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
#include "memory_map.h"
#include "halt.h"

const MemMap* UVISOR_WEAK memory_map_name(uint32_t addr)
{
    HALT_ERROR(NOT_IMPLEMENTED,
               "debug_mpu_config needs hw-specific implementation\n\r");
    return NULL;
}
