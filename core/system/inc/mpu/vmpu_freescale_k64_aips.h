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
#ifndef __VMPU_FREESCALE_K64_AIPS_H__
#define __VMPU_FREESCALE_K64_AIPS_H__

extern int vmpu_aips_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl);
extern void vmpu_aips_switch(uint8_t src_box, uint8_t dst_box);

#endif/*__VMPU_FREESCALE_K64_AIPS_H__*/
