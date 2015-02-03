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
#include <uvisor.h>
#include "svc.h"
#include "vmpu.h"

TBoxCx    g_svc_cx_state[SVC_CX_MAX_DEPTH];
uint32_t  g_svc_cx_state_ptr;
uint32_t *g_svc_cx_last_sp[SVC_CX_MAX_BOXES];
uint8_t   g_svc_cx_curr_id;

void svc_cx_thunk(void)
{
    /* FIXME the SVC_GW macro will change */
    SVC_GW(-1, svc_cx_thunk);
}

void svc_cx_switch_in(uint32_t *svc_sp,  uint32_t svc_pc,
                      uint8_t   svc_imm)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_fn;

    /* gather information from secure gateway */
    svc_gw_check_magic((TSecGw *) svc_pc);
    dst_fn = svc_gw_get_dst_fn((TSecGw *) svc_pc);
    dst_id = svc_gw_get_dst_id(svc_imm);

    /* different behavior with svc_imm == 0x80 */
    if(svc_gw_oop_mode(svc_imm))
    {
        /* TODO */
        while(1);
    }

    /* gather information from current state */
    src_sp = svc_cx_validate_sf(svc_sp);
    src_id = svc_cx_get_curr_id();
    dst_sp = svc_cx_get_last_sp(dst_id);

    /* switch exception stack frame */
    dst_sp = svc_cx_create_sf(src_sp, dst_sp, (uint32_t) svc_cx_thunk, dst_fn);

    /* save the current state */
    svc_cx_push(src_id, src_sp, dst_id);

    /* switch ACls */
    vmpu_switch(dst_id);
    __set_PSP((uint32_t) dst_sp);
}

void svc_cx_switch_out(uint32_t *svc_sp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

    /* gather information from current state */
    dst_id = svc_cx_get_curr_id();
    src_id = svc_cx_get_src_id();
    dst_sp = svc_cx_validate_sf(svc_sp);
    src_sp = svc_cx_get_src_sp();

    /* pop state */
    svc_cx_pop(dst_id, dst_sp, src_id);

    /* switch stack frames back */
    svc_cx_return_sf(src_sp, dst_sp);

    /* switch ACls */
    vmpu_switch(src_id);
    __set_PSP((uint32_t) src_sp);
}

void svc_cx_isr_in(void)
{
}

void svc_cx_isr_out(void)
{
}
