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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <uvisor.h>
#include "halt.h"

void debug_fault(THaltError reason, uint32_t lr);
void debug_cx_switch_in(void);
void debug_cx_switch_out(void);
void debug_print_mpu_config(void);
void debug_init(void);

#ifdef  NDEBUG

#define DEBUG_FAULT(...)         {}
#define DEBUG_CX_SWITCH_IN(...)  {}
#define DEBUG_CX_SWITCH_OUT(...) {}
#define DEBUG_PRINT_MPU_CFG(...) {}
#define DEBUG_INIT(...)          {}

#else /*NDEBUG*/

#define DEBUG_FAULT(reason) {\
    /************************************************************************/\
    /* lr is used to check execution mode before exception                  */\
    /* NOTE: this only works if the function is executed before any branch  */\
    /*       instruction right after the exception                          */\
    register uint32_t lr asm("lr");\
    /************************************************************************/\
    debug_fault(reason, lr);                                                  \
}
#define DEBUG_CX_SWITCH_IN  debug_cx_switch_in
#define DEBUG_CX_SWITCH_OUT debug_cx_switch_out
#define DEBUG_PRINT_MPU_CFG debug_print_mpu_config
#define DEBUG_INIT debug_init

#endif/*NDEBUG*/

#endif/*__DEBUG_H__*/
