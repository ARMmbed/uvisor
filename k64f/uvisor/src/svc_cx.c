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
#include "debug.h"

/* state variables */
TBoxCx    g_svc_cx_state[SVC_CX_MAX_DEPTH];
int       g_svc_cx_state_ptr;
uint32_t *g_svc_cx_curr_sp[SVC_CX_MAX_BOXES];
uint8_t   g_svc_cx_curr_id;
uint32_t  g_svc_cx_box_num;

void __attribute__((naked)) svc_cx_thunk(void)
{
    asm volatile(
        "svc %[svc_imm]\n"
        :: [svc_imm] "i" (SVC_MODE_UNPRIV_SVC_CX_OUT)
    );
}

void svc_cx_switch_in(uint32_t *svc_sp,  uint32_t svc_pc,
                      uint8_t   svc_imm)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_fn;
    uint32_t           dst_sp_align;

    /* gather information from secure gateway */
    svc_gw_check_magic((TSecGw *) svc_pc);
    dst_fn = svc_gw_get_dst_fn((TSecGw *) svc_pc);
    dst_id = svc_gw_get_dst_id((TSecGw *) svc_pc);

    /* gather information from current state */
    src_sp = svc_cx_validate_sf(svc_sp);
    src_id = svc_cx_get_curr_id();
    dst_sp = svc_cx_get_curr_sp(dst_id);

    /* check src and dst IDs */
    if(src_id == dst_id)
        HALT_ERROR("src_id == dst_id at box %i", src_id);

    /* create exception stack frame */
    dst_sp_align = ((uint32_t) dst_sp & 0x4) ? 1 : 0;
    dst_sp      -= (SVC_CX_EXC_SF_SIZE + dst_sp_align);

    /* populate exception stack frame:
     *   - scratch registers are copied
     *   - r12 is cleared
     *   - lr is the thunk function to close the secure gateway
     *   - return address is the destination function */
    memcpy((void *) dst_sp, (void *) src_sp,
           sizeof(uint32_t) * 4);                         /* r0 - r3          */
    dst_sp[4] = 0;                                        /* r12              */
    dst_sp[5] = (uint32_t) svc_cx_thunk;                  /* lr               */
    dst_sp[6] = dst_fn;                                   /* return address   */
    dst_sp[7] = src_sp[7] | (dst_sp_align << 9);          /* xPSR - alignment */

    /* save the current state */
    svc_cx_push_state(src_id, src_sp, dst_id, dst_sp);
    DEBUG_PRINT_SVC_CX_STATE();

    /* switch boxes */
    vmpu_switch(dst_id);
    __set_PSP((uint32_t) dst_sp);
}

void svc_cx_switch_out(uint32_t *svc_sp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

    /* gather information from current state */
    dst_sp = svc_cx_validate_sf(svc_sp);
    dst_id = svc_cx_get_curr_id();

    /* gather information from previous state */
    svc_cx_pop_state(dst_id, dst_sp);
    src_id = svc_cx_get_src_id();
    src_sp = svc_cx_get_src_sp();
    DEBUG_PRINT_SVC_CX_STATE();

    /* copy return value from destination stack frame */
    src_sp[0] = dst_sp[0];
    /* the destination stack frame gets discarded automatically since the
     * corresponding stack pointer is not stored anywhere */

    /* switch ACls */
    vmpu_switch(src_id);
    __set_PSP((uint32_t) src_sp);
}
