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

#define TACL_READ          0x0001
#define TACL_WRITE         0x0002
#define TACL_EXECUTE       0x0004
#define TACL_SIZE_ROUND_UP 0x0008
#define TACL_PERIPHERAL    TACL_SIZE_ROUND_UP

typedef uint32_t TACL;

typedef struct
{
    const void* start;
    uint32_t length;
    TACL acl;
} TACL_Entry;

typedef struct
{
    uint32_t stack;
    uint32_t heap;
} TBoxRegionSize;

typedef struct
{
    uint32_t magic;
    TBoxRegionSize size;
    const TACL_Entry* const acl_list;
    uint32_t acl_count;

    const void** const fn_list;
    uint32_t fn_count;
} TBoxDesc;

extern int vmpu_init(void);

extern int vmpu_acl_dev(TACL acl, uint16_t device_id);
extern int vmpu_acl_mem(TACL acl, uint32_t addr, uint32_t size);
extern int vmpu_acl_reg(TACL acl, uint32_t addr, uint32_t rmask, uint32_t wmask);
extern int vmpu_acl_bit(TACL acl, uint32_t addr);

extern bool vmpu_valid_code_addr(const void* address);

extern int vmpu_wr(uint32_t addr, uint32_t data);
extern int vmpu_rd(uint32_t addr);

extern int vmpu_box_add(const TBoxDesc *box);
extern int vmpu_fn_box(uint32_t addr);
extern int vmpu_switch(uint8_t box);

#define VMPU_REGION_SIZE(p1,p2) ((p1>=p2)?0:(((uint32_t)p2)-((uint32_t)p1)))

#endif/*__VMPU_H__*/
