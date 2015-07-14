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
#ifndef __PLATFORMS_H__
#define __PLATFORMS_H__

/* list of supported platforms */
#if defined(TARGET_LIKE_FRDM_K64F_GCC)         || \
    defined(TARGET_LIKE_STM32F429I_DISCO_GCC)

#define UVISOR_PRESENT

#endif

#endif/*__PLATFORMS_H__*/
