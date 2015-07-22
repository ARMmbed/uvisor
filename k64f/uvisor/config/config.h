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
#define HW_IRQ_VECTORS 86

#define XTAL_FREQUENCY 48000000UL

#define USE_FLASH_SIZE UVISOR_FLASH_SIZE
#define USE_SRAM_SIZE  UVISOR_SRAM_SIZE

#define STACK_SIZE 2048
#define STACK_GUARD_BAND 128
#define TOTAL_SRAM_SIZE (256*1024UL)

#endif/*__CONFIG_H__*/
