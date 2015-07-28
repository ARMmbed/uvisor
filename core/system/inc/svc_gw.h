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
#ifndef __SVC_GW_H__
#define __SVC_GW_H__

#include <uvisor.h>
#include <vmpu.h>
#include "svc_exports.h"
#include "svc_gw_exports.h"

#define UVISOR_SVC_GW_OPCODE (uint16_t) (0xDF00 +                              \
                                         UVISOR_SVC_ID_SECURE_GATEWAY(0))

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
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway not in flash (0x%08X)", svc_pc);

    if((svc_pc->opcode & ~((uint16_t) UVISOR_SVC_CUSTOM_MSK)) !=
        UVISOR_SVC_GW_OPCODE)
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway opcode invalid (0x%08X)\n", &svc_pc->opcode);

    if(svc_pc->magic != UVISOR_SVC_GW_MAGIC)
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway magic invalid (0x%08X)\n", &svc_pc->magic);
}

static inline uint32_t svc_gw_get_dst_fn(TSecGw *svc_pc)
{
    return svc_pc->dst_fn;
}

static inline uint8_t svc_gw_get_dst_id(TSecGw *svc_pc)
{
    uint32_t box_id = svc_pc->cfg_ptr - __uvisor_config.cfgtbl_start;

    if(box_id <= 0 || box_id >= g_vmpu_box_count)
        HALT_ERROR(SANITY_CHECK_FAILED, "box_id out of range (%i)", box_id);

    return (uint8_t) (box_id & 0xFF);
}

#endif/*__SVC_GW_H__*/
