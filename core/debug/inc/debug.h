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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <uvisor.h>
#include "halt.h"

void debug_init(void);
void debug_cx_switch_in(void);
void debug_cx_switch_out(void);
void debug_exc_sf(uint32_t lr);
void debug_fault_memmanage(void);
void debug_fault_bus(void);
void debug_fault_usage(void);
void debug_fault_hard(void);
void debug_fault_debug(void);
void debug_fault(THaltError reason, uint32_t lr);
void debug_mpu_config(void);
void debug_mpu_fault(void);
void debug_map_addr_to_periph(uint32_t address);

#define DEBUG_PRINT_HEAD(x) {\
    DPRINTF("\n***********************************************************\n");\
    DPRINTF("                    "x"\n");\
    DPRINTF("***********************************************************\n\n");\
}
#define DEBUG_PRINT_END()   {\
    DPRINTF("***********************************************************\n\n");\
}

#ifdef  NDEBUG

#define DEBUG_INIT(...)          {}
#define DEBUG_FAULT(...)         {}
#define DEBUG_CX_SWITCH_IN(...)  {}
#define DEBUG_CX_SWITCH_OUT(...) {}

#else /*NDEBUG*/

#define DEBUG_INIT debug_init
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

#endif/*NDEBUG*/

#endif/*__DEBUG_H__*/
