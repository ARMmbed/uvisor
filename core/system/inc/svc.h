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
#ifndef __SVC_H__
#define __SVC_H__

#include "svc_exports.h"

#include "svc_cx.h"
#include "svc_gw.h"

/* bitbanded address from regular address and bit position */
#define BITBAND_ADDRESS(Reg,Bit) ((uint32_t volatile*)(0x42000000u +\
    (32u*((uint32_t)(Reg) - (uint32_t)0x40000000u))                +\
    (4u*((uint32_t)(Bit)))))

void svc_init(void);

#endif/*__SVC_H__*/
