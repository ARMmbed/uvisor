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
#ifndef __UVISOR_H__
#define __UVISOR_H__

#ifdef  __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif/*__CPP__*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uvisor-config.h"

#ifndef TRUE
#define TRUE 1
#endif/*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif/*FALSE*/


#include <tfp_printf.h>
#ifndef dprintf
#define dprintf tfp_printf
#endif/*dprintf*/

/* unprivileged box as called by privileged code */
typedef void (*UnprivilegedBoxEntry)(void);

#define NOINLINE __attribute__ ((noinline))
#define PACKED __attribute__((packed))
#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

/* array count macro */
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

/* device-specific definitions */
#include "uvisor-device.h"

/* per-project-configuration */
#include <config.h>

#ifdef  NDEBUG
#define DPRINTF(...) {}
#define assert(...)
#else /*NDEBUG*/
#define DPRINTF dprintf
#define assert(x) if(!(x)){dprintf("HALTED(%s:%i): assert(%s)\n",  __FILE__, __LINE__, #x);while(1);}
#endif/*NDEBUG*/

#ifdef  CHANNEL_DEBUG
#if (CHANNEL_DEBUG<0) || (CHANNEL_DEBUG>31)
#error "Debug channel needs to be >=0 and <=31"
#endif
#endif/*CHANNEL_DEBUG*/

/* system wide error codes  */
#include <iot-error.h>

/* MPU definitions */
#ifdef  EFM32GG
#if (__MPU_PRESENT == 1)
#define MPU_REGIONS 8
#endif
#endif/*EFM32GG*/

/* IOT-OS specific includes */
#include <isr.h>
#include <linker.h>

#endif/*__UVISOR_H__*/
