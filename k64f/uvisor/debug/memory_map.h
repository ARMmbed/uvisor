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

#define MEMORY_MAP_NUM              90
#define MEMORY_MAP_PERIPH_START     ((uint32_t) 0x40000000)
#define MEMORY_MAP_PERIPH_END       ((uint32_t) 0x400FEFFF)
#define MEMORY_MAP_BITBANDING_START ((uint32_t) 0x42000000)
#define MEMORY_MAP_BITBANDING_END   ((uint32_t) 0x43FFFFFF)

typedef struct mem_map {
    char name[12];
    uint32_t base;
    uint32_t end;
} MemMap;

extern const MemMap g_mem_map[MEMORY_MAP_NUM];
extern const MemMap* memory_map_name(uint32_t addr);

#endif/*__MEMORY_MAP_H__*/
