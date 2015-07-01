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
#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <uvisor.h>

typedef struct mem_map {
    char name[15];
    uint32_t base;
    uint32_t end;
} MemMap;

extern const MemMap g_mem_map[];
const MemMap* memory_map_name(uint32_t addr);

#endif/*__MEMORY_MAP_H__*/
