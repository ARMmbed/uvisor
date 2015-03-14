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
#ifndef __HALT_H__
#define __HALT_H__

#ifdef  NDEBUG
#define HALT_ERROR(...) while(1)
#else /*NDEBUG*/
#define HALT_ERROR(...) halt_line(__FILE__, __LINE__, __VA_ARGS__);
#endif/*NDEBUG*/

extern void halt_error (const char *fmt, ...);
extern void halt_line (const char* file, uint32_t line, const char *fmt, ...);

#endif/*__HALT_H__*/
