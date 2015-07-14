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
#ifndef __UVISOR_LIB_H__
#define __UVISOR_LIB_H__

#include <stdint.h>

/* the symbol UVISOR_PRESENT is defined here based on the supported platforms */
#include "uvisor-lib/platforms.h"

/* these header files are included independently from the platform */
#include "uvisor-lib/uvisor_exports.h"
#include "uvisor-lib/vmpu_exports.h"

/* conditionally included header files */
#ifdef  UVISOR_PRESENT

#include "uvisor-lib/benchmark.h"
#include "uvisor-lib/bitband.h"
#include "uvisor-lib/config.h"
#include "uvisor-lib/interrupts.h"
#include "uvisor-lib/secure_gateway.h"
#include "uvisor-lib/svc_exports.h"
#include "uvisor-lib/svc_gw_exports.h"

#else /*UVISOR_PRESENT*/

#include "uvisor-lib/unsupported.h"

#endif/*UVISOR_PRESENT*/

#endif/*__UVISOR_LIB_H__*/
