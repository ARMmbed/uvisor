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
#ifndef __UVISOR_DEVICE_H__
#define __UVISOR_DEVICE_H__

#if   defined(EFM32GG)
#  include <em_device.h>
#elif defined(MK64F)
#  include <MK64F12.h>
#else
#  error "unknown ARCH in Makefile"
#endif

#endif/*__UVISOR_DEVICE_H__*/
