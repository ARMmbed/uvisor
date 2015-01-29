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
#include "svc_cx.h"
#include "svc_gw.h"
#include "vmpu.h"

TBoxTh  g_svc_cx[SVC_CX_MAX_DEPTH];
TBoxId  g_svc_cx_ptr;

/* FIXME this goes somewhere else */
TBoxSp *g_svc_stack[SVC_CX_MAX_BOXES];
TBoxId  g_box_num;

void svc_cx_thunk(void)
{
    /* FIXME the SVC_GW macro will change */
    SVC_GW(-1, svc_cx_thunk);
}

void svc_cx_switch_in(TBoxSp  *svc_sp,  uint16_t *svc_pc,
                      uint8_t  svc_imm)
{
    TBoxId  src_id,  dst_id;
    TBoxSp *src_sp, *dst_sp;
    TBoxFn           dst_fn;

    /* gather information from secure gateway */
    svc_gw_check_magic(svc_pc);
    dst_id = (TBoxId) svc_gw_get_dst_id(svc_imm);
    dst_fn = (TBoxFn) svc_gw_get_dst_fn(svc_pc);

    /* different behavior with svc_imm == 0x80 */
    if(svc_gw_oop_mode(svc_imm))
    {
        /* TODO */
    }

    /* gather information from current state */
    src_sp = svc_cx_validate_sf(svc_sp);
    src_id = svc_cx_get_dst_id();
    dst_sp = svc_cx_get_last_sp(dst_id);

    /* switch exception stack frame */
    dst_sp = svc_cx_switch_sf(src_sp, dst_sp, (TBoxFn) svc_cx_thunk, dst_fn);

    /* switch ACls */
    vmpu_switch(dst_id);

    /* save the current state */
    svc_cx_push(src_id, dst_id, src_sp, dst_sp);
}

void svc_cx_switch_out(TBoxSp *svc_sp)
{
    TBoxId  src_id,  dst_id;
    TBoxSp *src_sp, *dst_sp;

    /* gather information from current state */
    dst_id = svc_cx_get_dst_id();
    src_id = svc_cx_get_src_id();
    dst_sp = svc_cx_get_dst_sp();
    src_sp = svc_cx_get_src_sp();

    /* check consistency between switch in and switch out */
    if(dst_sp != svc_cx_validate_sf(svc_sp))
    {
        /* FIXME raise fault */
        while(1);
    };

    /* pop state */
    svc_cx_pop(dst_id, dst_sp);

    /* switch stack frames back */
    svc_cx_return_sf(src_sp, dst_sp);

    /* switch ACls */
    vmpu_switch(src_id);
}

void svc_cx_isr_in(void)
{
}

void svc_cx_isr_out(void)
{
}
