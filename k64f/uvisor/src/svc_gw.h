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
#ifndef __SVC_GW_H__
#define __SVC_GW_H__

#include <uvisor.h>
#include "svc_gw_exports.h"

#define SVC_GW_BOX_ID_Msk  ((uint8_t) 0x3FU)
#define SVC_GW_BOX_OOP_Msk ((uint8_t) 0x40U)

#define SVC_GW_InFlash_Msk   ((uint32_t) ~(FLASH_ORIGIN + FLASH_LENGTH - 1))
#define SVC_GW_InFlash(addr) ((uint32_t) (addr) & SVC_GW_InFlash_Msk)

typedef struct {
    uint16_t opcode;
    uint16_t branch;
    uint32_t magic;
    uint32_t dst_fn;
    uint32_t *cfg_ptr;
} UVISOR_PACKED TSecGw;

static inline void svc_gw_check_magic(TSecGw *svc_pc)
{
    if(SVC_GW_InFlash((uint32_t) svc_pc))
    {
        /* FIXME raise fault */
        while(1);
    }
    if(svc_pc->magic != UVISOR_SVC_GW_MAGIC)
    {
        /* FIXME raise fault */
        while(1);
    }
}

static inline uint32_t svc_gw_get_dst_fn(TSecGw *svc_pc)
{
    return svc_pc->dst_fn;
}

static inline uint8_t svc_gw_get_dst_id(TSecGw *svc_pc)
{
    uint32_t box_id = (uint32_t) (svc_pc->cfg_ptr -
                                  __uvisor_config.cfgtbl_start);
    if(box_id >= 0 && box_id < g_svc_cx_box_num)
    {
        return (uint8_t) (box_id & 0xFF);
    }
    else
    {
        /* FIXME raise fault */
        while(1);
    }
}

static inline uint8_t svc_gw_oop_mode(uint8_t svc_imm)
{
    return svc_imm & SVC_GW_BOX_OOP_Msk;
}
#endif/*__SVC_GW_H__*/
