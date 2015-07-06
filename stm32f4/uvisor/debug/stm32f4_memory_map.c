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
#include "memory_map.h"
#include "stm32f4_memory_map.h"

const MemMap g_mem_map[] = {
};

const MemMap* memory_map_name(uint32_t addr)
{
    int i;
    const MemMap *map;

    /* find system memory region */
    map = g_mem_map;
    for(i = 0; i < UVISOR_ARRAY_COUNT(g_mem_map); i++)
        if((addr >= map->base) && (addr <= map->end))
            return map;
        else
            map++;

    return NULL;
}
