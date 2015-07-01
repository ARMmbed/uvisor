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
#ifndef __K64F_MEMORY_MAP_H__
#define __K64F_MEMORY_MAP_H__

#include <uvisor.h>

#define MEMORY_MAP_SRAM_START       ((uint32_t) SRAM_ORIGIN)
#define MEMORY_MAP_SRAM_END         ((uint32_t) (SRAM_ORIGIN + SRAM_LENGTH))
#define MEMORY_MAP_PERIPH_START     ((uint32_t) 0x40000000)
#define MEMORY_MAP_PERIPH_END       ((uint32_t) 0x400FEFFF)
#define MEMORY_MAP_GPIO_START       ((uint32_t) 0x400FF000)
#define MEMORY_MAP_GPIO_END         ((uint32_t) 0x400F0000)
#define MEMORY_MAP_BITBANDING_START ((uint32_t) 0x42000000)
#define MEMORY_MAP_BITBANDING_END   ((uint32_t) 0x43FFFFFF)

#endif/*__K64F_MEMORY_MAP_H__*/
