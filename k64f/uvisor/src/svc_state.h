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
#ifndef __SVC_STATE_H__
#define __SVC_STATE_H__

#include <uvisor.h>
#include <uvisor-lib.h>

#define SVC_STATE_EXC_SF_SIZE 8
#define SVC_STATE_EXC_DW_Pos  9
#define SVC_STATE_EXC_DW(x)   ((uint32_t) (x) << SVC_STATE_EXC_DW_Pos)
#define SVC_STATE_EXC_DW_Msk  SVC_STATE_EXC_DW(1)

#define SVC_STATE_LR_DW_Pos  2
#define SVC_STATE_LR_DW(x)   ((uint32_t) (x) << SVC_STATE_LR_DW_Pos)
#define SVC_STATE_LR_DW_Msk  SVC_STATE_LR_DW(1)

#define SVC_STATE_MAX_BOXES   0x80U
#define SVC_STATE_MAX_DEPTH   0x10U

typedef uint8_t  SecBoxId;
typedef uint32_t SecBoxSp;
typedef uint32_t SecBoxFn;

typedef struct{
    SecBoxId  dst_id;
    SecBoxId  src_id;
    SecBoxSp *dst_sp;
    SecBoxSp *src_sp;
} UVISOR_PACKED SecBoxTh;

extern SecBoxTh  g_svc_state[SVC_STATE_MAX_DEPTH];
extern SecBoxSp *g_svc_stack[SVC_STATE_MAX_BOXES];
extern SecBoxId  g_svc_state_ptr;
extern SecBoxId  g_box_num; /* FIXME this goes somewhere else */

static inline SecBoxId svc_state_get_src_id()
{
    return g_svc_state[g_svc_state_ptr].src_id;
}

static inline SecBoxId svc_state_get_dst_id()
{
    return g_svc_state[g_svc_state_ptr].dst_id;
}

static inline SecBoxSp *svc_state_get_dst_sp()
{
    return g_svc_state[g_svc_state_ptr].dst_sp;
}

static inline SecBoxSp *svc_state_get_src_sp()
{
    return g_svc_state[g_svc_state_ptr].src_sp;
}

static inline SecBoxSp *svc_state_get_last_sp(SecBoxId box_id)
{
    return g_svc_stack[box_id];
}

static inline void svc_state_set_last_sp(SecBoxId box_id, SecBoxSp *box_sp)
{
    g_svc_stack[box_id] = box_sp;
}

static void inline svc_state_push(SecBoxId  src_id, SecBoxId  dst_id,
                                  SecBoxSp *src_sp, SecBoxSp *dst_sp)
{
    /* check state stack overflow */
    if(g_svc_state_ptr == SVC_STATE_MAX_DEPTH)
    {
        /* FIXME raise fault */
        while(1);
    }
    else
    {
        ++g_svc_state_ptr;
    }

    /* save state */
    g_svc_state[g_svc_state_ptr].dst_id = dst_id;
    g_svc_state[g_svc_state_ptr].src_id = src_id;
    g_svc_state[g_svc_state_ptr].dst_sp = dst_sp;
    g_svc_state[g_svc_state_ptr].src_sp = src_sp;

    /* save last used stack pointer for the src box */
    svc_state_set_last_sp(src_id, src_sp);
}

static inline void svc_state_pop(SecBoxId dst_id, SecBoxSp *dst_sp)
{
    /* check state stack underflow */
    if(!g_svc_state_ptr)
    {
        /* FIXME raise fault */
        while(1);
    }
    else
    {
        --g_svc_state_ptr;
    }

    /* save last used stack pointer for the dst box */
    SecBoxSp dst_sp_align= (dst_sp[7] & SVC_STATE_EXC_DW_Msk) ? 1 : 0;
    svc_state_set_last_sp(dst_id,
                          dst_sp + SVC_STATE_EXC_SF_SIZE + dst_sp_align);
}

static inline SecBoxSp *svc_state_validate_sf(SecBoxSp *sp)
{
    /* the box stack pointer is validated through direct access: if it has been
     * tampered with access is proihibited by the uvisor and a fault occurs;
     * the approach is applied to the whole stack frame:
     *   from sp[0] to sp[7] if the stack is double word aligned
     *   from sp[0] to sp[8] if the stack is not double word aligned */
    uint32_t tmp = 0;
    asm volatile(
        "ldrt   %[tmp], [%[sp]]\n"                     /* test sp[0] */
        "ldrt   %[tmp], [%[sp], %[exc_sf_size]]\n"     /* test sp[7] */
        "tst    %[tmp], %[exc_sf_dw_msk]\n"            /* test alignment */
        "it     ne\n"
        "ldrtne %[tmp], [%[sp], %[max_exc_sf_size]]\n" /* test sp[8] */
        : [tmp]             "=r" (tmp)
        : [sp]              "r"  ((uint32_t) sp),
          [exc_sf_dw_msk]   "i"  ((uint32_t) SVC_STATE_EXC_DW_Msk),
          [exc_sf_size]     "i"  ((uint32_t) (sizeof(SecBoxSp) *
                                              (SVC_STATE_EXC_SF_SIZE - 1))),
          [max_exc_sf_size] "i"  ((uint32_t) (sizeof(SecBoxSp) *
                                              (SVC_STATE_EXC_SF_SIZE)))
    );

    /* the initial stack pointer, if validated, is returned unchanged */
    return sp;
}

static inline SecBoxSp *svc_state_switch_sf(SecBoxSp *src_sp, SecBoxSp *dst_sp,
                                            SecBoxFn  thunk,  SecBoxSp  dst_fn)
{
    SecBoxSp  dst_sf_align;
    SecBoxSp *dst_sf;

    /* create destination stack frame */
    dst_sf_align = ((SecBoxSp) dst_sp & SVC_STATE_LR_DW_Msk) ? 1 : 0;
    dst_sf       = dst_sp - SVC_STATE_EXC_SF_SIZE - dst_sf_align;

    /* populate destination stack frame:
     *   r0 - r3         are copied
     *   r12             is cleared
     *   lr              is the thunk function
     *   return address  is the destination function
     *   xPSR            is modified to account for stack alignment */
    memcpy((void *) dst_sf, (void *) src_sp, sizeof(SecBoxSp) * 4);
    dst_sf[4] = 0;
    dst_sf[5] = thunk;
    dst_sf[6] = dst_fn;
    dst_sf[7] = src_sp[7] | SVC_STATE_EXC_DW(dst_sf_align);

    /* destination stack frame selected */
    __set_PSP((uint32_t) dst_sf);
    return dst_sf;
}

static inline void svc_state_return_sf(SecBoxSp *src_sp, SecBoxSp *dst_sp)
{
    /* copy return value to source stack */
    src_sp[0] = dst_sp[0];

    /* revert back to source stack */
    __set_PSP((uint32_t) src_sp);
}
#endif/*__SVC_STATE_H__*/
