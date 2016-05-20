/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __SVC_GW_H__
#define __SVC_GW_H__

#include <uvisor.h>
#include "vmpu.h"
#include "api/inc/svc_exports.h"
#include "api/inc/svc_gw_exports.h"

#define UVISOR_SVC_GW_OPCODE (uint16_t) (0xDF00 +                              \
                                         UVISOR_SVC_ID_SECURE_GATEWAY(0))

typedef struct {
    uint16_t opcode;
    uint16_t branch;
    uint32_t magic;
    uint32_t dst_fn;
    uint32_t *cfg_ptr;
} UVISOR_PACKED TSecGw;

static UVISOR_FORCEINLINE void svc_gw_check_magic(TSecGw *svc_pc)
{
    if(!vmpu_public_flash_addr((uint32_t) svc_pc))
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway not in flash (0x%08X)", svc_pc);

    if((svc_pc->opcode & ~((uint16_t) UVISOR_SVC_FAST_NARGS_MASK)) != UVISOR_SVC_GW_OPCODE)
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway opcode invalid (0x%08X)\n", &svc_pc->opcode);

    if(svc_pc->magic != UVISOR_SVC_GW_MAGIC)
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "secure gateway magic invalid (0x%08X)\n", &svc_pc->magic);
}

static UVISOR_FORCEINLINE uint32_t svc_gw_get_dst_fn(TSecGw *svc_pc)
{
    return svc_pc->dst_fn;
}

static UVISOR_FORCEINLINE uint8_t svc_gw_get_dst_id(TSecGw *svc_pc)
{
    uint8_t box_id;

    box_id = (uint8_t) ((svc_pc->cfg_ptr - __uvisor_config.cfgtbl_ptr_start) & 0xFF);

    /* We explicitly check that box_id is not 0 as we currently do not allow
     * secure gateways to box 0. */
    if (!box_id || !vmpu_is_box_id_valid(box_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Secure gateway: The box ID is out of range (%i).\r\n", box_id);
    }

    return box_id;
}

#endif/*__SVC_GW_H__*/
