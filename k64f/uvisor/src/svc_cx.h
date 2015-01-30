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
#ifndef __SVC_CX_H__
#define __SVC_CX_H__

#include <uvisor.h>
#include <uvisor-lib.h>
#include "vmpu.h"

#define SVC_CX_EXC_SF_SIZE 8
#define SVC_CX_EXC_DW_Pos  9
#define SVC_CX_EXC_DW(x)   ((uint32_t) (x) << SVC_CX_EXC_DW_Pos)
#define SVC_CX_EXC_DW_Msk  SVC_CX_EXC_DW(1)

#define SVC_CX_LR_DW_Pos  2
#define SVC_CX_LR_DW(x)   ((uint32_t) (x) << SVC_CX_LR_DW_Pos)
#define SVC_CX_LR_DW_Msk  SVC_CX_LR_DW(1)

#define SVC_CX_MAX_BOXES   0x80U
#define SVC_CX_MAX_DEPTH   0x10U

typedef struct {
    TBoxId  dst_id;
    TBoxId  src_id;
    TBoxSp *dst_sp;
    TBoxSp *src_sp;
} TBoxTh;

extern TBoxTh  g_svc_cx[SVC_CX_MAX_DEPTH];
extern TBoxId  g_svc_cx_ptr;

/* FIXME this goes somewhere else */
extern TBoxSp *g_svc_stack[SVC_CX_MAX_BOXES];
extern TBoxId  g_box_num;

static inline TBoxId svc_cx_get_src_id()
{
    return g_svc_cx[g_svc_cx_ptr].src_id;
}

static inline TBoxId svc_cx_get_dst_id()
{
    return g_svc_cx[g_svc_cx_ptr].dst_id;
}

static inline TBoxSp *svc_cx_get_dst_sp()
{
    return g_svc_cx[g_svc_cx_ptr].dst_sp;
}

static inline TBoxSp *svc_cx_get_src_sp()
{
    return g_svc_cx[g_svc_cx_ptr].src_sp;
}

static inline TBoxSp *svc_cx_get_last_sp(TBoxId box_id)
{
    return g_svc_stack[box_id];
}

static inline void svc_cx_set_last_sp(TBoxId box_id, TBoxSp *box_sp)
{
    g_svc_stack[box_id] = box_sp;
}

static void inline svc_cx_push(TBoxId  src_id, TBoxId  dst_id,
                               TBoxSp *src_sp, TBoxSp *dst_sp)
{
    /* check state stack overflow */
    if(g_svc_cx_ptr == SVC_CX_MAX_DEPTH)
    {
        /* FIXME raise fault */
        while(1);
    }
    else
    {
        ++g_svc_cx_ptr;
    }

    /* save state */
    g_svc_cx[g_svc_cx_ptr].dst_id = dst_id;
    g_svc_cx[g_svc_cx_ptr].src_id = src_id;
    g_svc_cx[g_svc_cx_ptr].dst_sp = dst_sp;
    g_svc_cx[g_svc_cx_ptr].src_sp = src_sp;

    /* save last used stack pointer for the src box */
    svc_cx_set_last_sp(src_id, src_sp);
}

static inline void svc_cx_pop(TBoxId dst_id, TBoxSp *dst_sp)
{
    /* check state stack underflow */
    if(!g_svc_cx_ptr)
    {
        /* FIXME raise fault */
        while(1);
    }
    else
    {
        --g_svc_cx_ptr;
    }

    /* save last used stack pointer for the dst box */
    TBoxSp dst_sp_align= (dst_sp[7] & SVC_CX_EXC_DW_Msk) ? 1 : 0;
    svc_cx_set_last_sp(dst_id,
                       dst_sp + SVC_CX_EXC_SF_SIZE + dst_sp_align);
}

static inline TBoxSp *svc_cx_validate_sf(TBoxSp *sp)
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
          [exc_sf_dw_msk]   "i"  ((uint32_t) SVC_CX_EXC_DW_Msk),
          [exc_sf_size]     "i"  ((uint32_t) (sizeof(TBoxSp) *
                                              (SVC_CX_EXC_SF_SIZE - 1))),
          [max_exc_sf_size] "i"  ((uint32_t) (sizeof(TBoxSp) *
                                              (SVC_CX_EXC_SF_SIZE)))
    );

    /* the initial stack pointer, if validated, is returned unchanged */
    return sp;
}

static inline TBoxSp *svc_cx_create_sf(TBoxSp *src_sp, TBoxSp *dst_sp,
                                       TBoxFn  thunk,  TBoxFn  dst_fn)
{
    TBoxSp  dst_sf_align;
    TBoxSp *dst_sf;

    /* create destination stack frame */
    dst_sf_align = ((TBoxSp) dst_sp & SVC_CX_LR_DW_Msk) ? 1 : 0;
    dst_sf       = dst_sp - SVC_CX_EXC_SF_SIZE - dst_sf_align;

    /* populate destination stack frame:
     *   r0 - r3         are copied
     *   r12             is cleared
     *   lr              is the thunk function
     *   return address  is the destination function
     *   xPSR            is modified to account for stack alignment */
    memcpy((void *) dst_sf, (void *) src_sp, sizeof(TBoxSp) * 4);
    dst_sf[4] = 0;
    dst_sf[5] = (TBoxSp) thunk;
    dst_sf[6] = (TBoxSp) dst_fn;
    dst_sf[7] = src_sp[7] | SVC_CX_EXC_DW(dst_sf_align);

    return dst_sf;
}

static inline void svc_cx_return_sf(TBoxSp *src_sp, TBoxSp *dst_sp)
{
    /* copy return value to source stack */
    src_sp[0] = dst_sp[0];
}
#endif/*__SVC_CX_H__*/
