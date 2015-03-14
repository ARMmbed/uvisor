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
#ifndef __UVISOR_EXPORTS_H__
#define __UVISOR_EXPORTS_H__

#ifdef  __cplusplus
#define UVISOR_EXTERN extern "C"
#else
#define UVISOR_EXTERN extern
#endif/*__CPP__*/

#define UVISOR_NOINLINE __attribute__ ((noinline))
#define UVISOR_PACKED   __attribute__((packed))
#define UVISOR_WEAK     __attribute__ ((weak))
#define UVISOR_ALIAS(f) __attribute__ ((weak, alias (#f)))
#define UVISOR_LINKTO(f) __attribute__ ((alias (#f)))
#define UVISOR_NORETURN__attribute__ ((noreturn))

/* array count macro */
#define UVISOR_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

#endif/*__UVISOR_EXPORTS_H__*/
