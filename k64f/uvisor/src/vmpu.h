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
#ifndef __VMPU_H__
#define __VMPU_H__

#include "vmpu_exports.h"

#define VMPU_FLASH_ADDR_MASK (~(((uint32_t)(FLASH_LENGTH)) - 1))
#define VMPU_FLASH_ADDR(addr) ((VMPU_FLASH_ADDR_MASK & (uint32_t)(addr)) == (FLASH_ORIGIN & VMPU_FLASH_ADDR_MASK))

extern int  vmpu_sanity_checks(void);
extern void vmpu_init(void);

extern int vmpu_acl_dev(UvisorBoxAcl acl, uint16_t device_id);
extern int vmpu_acl_mem(UvisorBoxAcl acl, uint32_t addr, uint32_t size);
extern int vmpu_acl_reg(UvisorBoxAcl acl, uint32_t addr, uint32_t rmask,
                        uint32_t wmask);
extern int vmpu_acl_bit(UvisorBoxAcl acl, uint32_t addr);

extern bool vmpu_valid_code_addr(const void* address);

extern int vmpu_wr(uint32_t addr, uint32_t data);
extern int vmpu_rd(uint32_t addr);

extern int vmpu_box_add(const UvisorBoxConfig *box);
extern int vmpu_fn_box(uint32_t addr);
extern int vmpu_switch(uint8_t box);

#define VMPU_REGION_SIZE(p1,p2) ((p1>=p2)?0:(((uint32_t)p2)-((uint32_t)p1)))

#endif/*__VMPU_H__*/
