/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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
#ifndef __SVC_CX_H__
#define __SVC_CX_H__

#include <uvisor.h>
#include "halt.h"
#include "vmpu.h"

#define SVC_CX_EXC_SF_SIZE 8

typedef enum {
    TBOXCX_INVALID = 0,
    TBOXCX_SECURE_GATEWAY,
    TBOXCX_IRQ,
    __TBOXCXTYPE_MAX /* The number of possible box context types */
} TBoxCxType;

typedef struct {
    uint8_t   rfu[2];  /* for 32 bit alignment */
    uint8_t   src_id;
    uint8_t   type;
    uint32_t *src_sp;
} UVISOR_PACKED TBoxCx;

/* state variables */
extern TBoxCx    g_svc_cx_state[UVISOR_SVC_CONTEXT_MAX_DEPTH];
extern int       g_svc_cx_state_ptr;
extern uint32_t *g_svc_cx_curr_sp[UVISOR_MAX_BOXES];
extern uint32_t *g_svc_cx_context_ptr[UVISOR_MAX_BOXES];
extern uint8_t g_active_box;

static UVISOR_FORCEINLINE uint8_t svc_cx_get_src_id(void)
{
    return g_svc_cx_state[g_svc_cx_state_ptr].src_id;
}

static UVISOR_FORCEINLINE uint32_t *svc_cx_get_src_sp(void)
{
    return g_svc_cx_state[g_svc_cx_state_ptr].src_sp;
}

static UVISOR_FORCEINLINE uint32_t *svc_cx_get_curr_sp(uint8_t box_id)
{
    return g_svc_cx_curr_sp[box_id];
}

static UVISOR_FORCEINLINE void svc_cx_push_state(uint8_t src_id, uint8_t type, uint32_t *src_sp, uint8_t dst_id)
{
    /* check state stack overflow */
    if(g_svc_cx_state_ptr == UVISOR_SVC_CONTEXT_MAX_DEPTH)
        HALT_ERROR(SANITY_CHECK_FAILED, "state stack overflow");

    /* push state */
    g_svc_cx_state[g_svc_cx_state_ptr].src_id = src_id;
    g_svc_cx_state[g_svc_cx_state_ptr].type = type;
    g_svc_cx_state[g_svc_cx_state_ptr].src_sp = src_sp;
    ++g_svc_cx_state_ptr;

    /* save curr stack pointer for the src box */
        g_svc_cx_curr_sp[src_id] = src_sp;

    /* update current box id */
    g_active_box = dst_id;
}

static UVISOR_FORCEINLINE void svc_cx_pop_state(uint8_t dst_id, uint32_t *dst_sp)
{
    /* check state stack underflow */
    if(!g_svc_cx_state_ptr)
        HALT_ERROR(SANITY_CHECK_FAILED, "state stack underflow");

    /* pop state */
    --g_svc_cx_state_ptr;

    /* save curr stack pointer for the dst box */
    uint32_t dst_sp_align = (dst_sp[7] & 0x4) ? 1 : 0;
    g_svc_cx_curr_sp[dst_id] = dst_sp + SVC_CX_EXC_SF_SIZE + dst_sp_align;

    /* update current box id */
    g_active_box = svc_cx_get_src_id();
}

uint32_t *svc_cx_validate_sf(uint32_t *sp);

#endif/*__SVC_CX_H__*/
