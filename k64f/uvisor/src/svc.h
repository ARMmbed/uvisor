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

/* SVC immediate values for call from unprivileged code */
#define SVC_MODE_UNPRIV_SVC_CUSTOM    (0x0 << 6)
#define SVC_MODE_UNPRIV_SVC_UNVIC_OUT (0x1 << 6)
#define SVC_MODE_UNPRIV_SVC_CX_IN     (0x2 << 6)
#define SVC_MODE_UNPRIV_SVC_CX_OUT    (0x3 << 6)

/* SVC immediate values for call from privileged code */
#define SVC_MODE_PRIV_SVC_CUSTOM_IN (0x0 << 6)
#define SVC_MODE_PRIV_SVC_UNVIC_IN  (0x1 << 6)

#include "svc_cx.h"
#include "svc_gw.h"

void svc_init(void);

#endif/*__SVC_H__*/
