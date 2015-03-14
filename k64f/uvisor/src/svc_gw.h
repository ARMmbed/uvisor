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
#include <vmpu.h>
#include "svc_gw_exports.h"

#define SVC_GW_BOX_ID_Msk  ((uint8_t) 0x3FU)
#define SVC_GW_BOX_OOP_Msk ((uint8_t) 0x40U)

typedef struct {
    uint16_t opcode;
    uint16_t branch;
    uint32_t magic;
    uint32_t dst_fn;
    uint32_t *cfg_ptr;
} UVISOR_PACKED TSecGw;

static inline void svc_gw_check_magic(TSecGw *svc_pc)
{
    if(!VMPU_FLASH_ADDR(svc_pc))
        HALT_ERROR("secure gateway not in flash (0x%08X)", svc_pc);

    if(svc_pc->magic != UVISOR_SVC_GW_MAGIC)
        HALT_ERROR("secure gateway magic invalid (0x%08X)\n", &svc_pc->magic);
}

static inline uint32_t svc_gw_get_dst_fn(TSecGw *svc_pc)
{
    return svc_pc->dst_fn;
}

static inline uint8_t svc_gw_get_dst_id(TSecGw *svc_pc)
{
    uint32_t box_id = (uint32_t) (svc_pc->cfg_ptr -
                                  __uvisor_config.cfgtbl_start);
    if(box_id <= 0 || box_id >= g_svc_cx_box_num)
        HALT_ERROR("box_id out of range (%i)", box_id);

    return (uint8_t) (box_id & 0xFF);
}

static inline uint8_t svc_gw_oop_mode(uint8_t svc_imm)
{
    return svc_imm & SVC_GW_BOX_OOP_Msk;
}
#endif/*__SVC_GW_H__*/
