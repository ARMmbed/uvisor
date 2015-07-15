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
#ifndef __HALT_H__
#define __HALT_H__

typedef enum {
    PERMISSION_DENIED = 1,
    SANITY_CHECK_FAILED,
    NOT_IMPLEMENTED,
    NOT_ALLOWED,
    FAULT_MEMMANAGE,
    FAULT_BUS,
    FAULT_USAGE,
    FAULT_HARD,
    FAULT_DEBUG
} THaltError;

#define HALT_INTRA_PATTERN_DELAY 0x200000U
#define HALT_INTER_PATTERN_DELAY (3*HALT_INTRA_PATTERN_DELAY)

#ifdef  NDEBUG
#define HALT_ERROR(reason, ...) halt_led(reason)
#else /*NDEBUG*/
#define HALT_ERROR(reason, ...) \
        halt_line(__FILE__, __LINE__, reason, __VA_ARGS__)
#endif/*NDEBUG*/

extern void halt_led(THaltError reason);
extern void halt_error(THaltError reason, const char *fmt, ...);
extern void halt_line(const char *file, uint32_t line, THaltError reason,
                      const char *fmt, ...);

#endif/*__HALT_H__*/
