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
#ifndef __BITBAND_H__
#define __BITBAND_H__

#define BITBAND_BASE       0x40000000
#define BITBAND_ALIAS_BASE 0x42000000

/* bitbanded address from regular address and bit position */
#define BITBAND_ADDRESS(reg, bit) ((uint32_t volatile *)(BITBAND_ALIAS_BASE +\
	(32u * ((uint32_t) (reg) - (uint32_t) BITBAND_BASE))                    +\
	( 4u * ((uint32_t) (bit)))))

/* APIs to access bitbanding from unprivileged code */
void bitband_write(uint32_t *addr, uint32_t val);

#endif/*__BITBAND_H__*/
