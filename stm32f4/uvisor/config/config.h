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
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <uvisor-config.h>

/* number of NVIC vectors (IRQ vectors, hardware specific) */
#define HW_IRQ_VECTORS 90

/* memory not to be used by uVisor linker script */
#define RESERVED_FLASH 0x400
#define RESERVED_SRAM  0x1AC

/* maximum memory used by uVisor */
#define USE_FLASH_SIZE UVISOR_FLASH_SIZE
#define USE_SRAM_SIZE  UVISOR_SRAM_SIZE

/* stack configuration */
#define STACK_SIZE       2048
#define STACK_GUARD_BAND  (STACK_SIZE / 8)

#endif/*__CONFIG_H__*/
