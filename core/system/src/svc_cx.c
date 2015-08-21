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
#include <uvisor.h>
#include "svc.h"
#include "vmpu.h"
#include "debug.h"

/* state variables */
TBoxCx    g_svc_cx_state[UVISOR_SVC_CONTEXT_MAX_DEPTH];
int       g_svc_cx_state_ptr;
uint32_t *g_svc_cx_curr_sp[UVISOR_MAX_BOXES];
uint32_t *g_svc_cx_context_ptr[UVISOR_MAX_BOXES];
uint8_t   g_svc_cx_curr_id;

void UVISOR_NAKED svc_cx_thunk(void)
{
    asm volatile(
        "svc %[svc_id]\n"
        :: [svc_id] "i" (UVISOR_SVC_ID_CX_OUT)
    );
}

uint32_t *svc_cx_validate_sf(uint32_t *sp)
{
    /* the box stack pointer is validated through direct access: if it has been
     * tampered with access is proihibited by the uvisor and a fault occurs;
     * the approach is applied to the whole stack frame:
     *   from sp[0] to sp[7] if the stack is double word aligned
     *   from sp[0] to sp[8] if the stack is not double word aligned */
    asm volatile(
        "ldrt   r1, [%[sp]]\n"                        /* test sp[0] */
        "ldrt   r1, [%[sp], %[exc_sf_size]]\n"        /* test sp[7] */
        "tst    r1, #0x4\n"                           /* test alignment */
        "it     ne\n"
        "ldrtne r1, [%[sp], %[max_exc_sf_size]]\n"    /* test sp[8] */
        :: [sp]              "r"  (sp),
           [exc_sf_size]     "i"  ((uint32_t) (sizeof(uint32_t) *
                                               (SVC_CX_EXC_SF_SIZE - 1))),
           [max_exc_sf_size] "i"  ((uint32_t) (sizeof(uint32_t) *
                                               (SVC_CX_EXC_SF_SIZE)))
         : "r1"
    );

    /* the initial stack pointer, if validated, is returned unchanged */
    return sp;
}

void UVISOR_NAKED svc_cx_switch_in(uint32_t *svc_sp, uint32_t *svc_pc,
                                   uint8_t   svc_id)
{
    asm volatile(
        "push {r4 - r11}\n"         /* store callee-saved regs on MSP (priv) */
        "push {lr}\n"
        // r0 = svc_sp              /* 1st arg: SVC sp */
        // r1 = svc_pc              /* 1st arg: SVC pc */
        // r2 = svc_id              /* 1st arg: SVC number */
        "bl   __svc_cx_switch_in\n" /* execute handler for context switch */
        "cmp  r0, #0\n"             /* check return flag */
        "beq  no_regs_clearing\n"
        "mov  r4,  #0\n"            /* clear callee-saved regs on request */
        "mov  r5,  #0\n"
        "mov  r6,  #0\n"
        "mov  r7,  #0\n"
        "mov  r8,  #0\n"
        "mov  r9,  #0\n"
        "mov  r10, #0\n"
        "mov  r11, #0\n"
        "no_regs_clearing:\n"
        "pop  {pc}\n"               /* callee-saved regs are not popped */
        /* the destination function will be executed after this */
        :: "r" (svc_sp), "r" (svc_pc), "r" (svc_id)
    );
}

uint32_t __svc_cx_switch_in(uint32_t *svc_sp, uint32_t svc_pc,
                            uint8_t   svc_id)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_fn;
    uint32_t           dst_sp_align;

    /* number of arguments to pass to the target function */
    uint32_t args = (uint32_t) (svc_id & UVISOR_SVC_CUSTOM_MSK);

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
        HALT_ERROR(NOT_ALLOWED, "src_id == dst_id at box %i", src_id);

    /* create exception stack frame */
    dst_sp_align = ((uint32_t) dst_sp & 0x4) ? 1 : 0;
    dst_sp      -= (SVC_CX_EXC_SF_SIZE + dst_sp_align);

    /* populate exception stack frame:
     *   - scratch registers are copied
     *   - r12 is cleared
     *   - lr is the thunk function to close the secure gateway
     *   - return address is the destination function */
    memcpy((void *) dst_sp, (void *) src_sp,
           sizeof(uint32_t) * args);                      /* r0 - r3          */
    dst_sp[4] = 0;                                        /* r12              */
    dst_sp[5] = (uint32_t) svc_cx_thunk;                  /* lr               */
    dst_sp[6] = dst_fn;                                   /* return address   */
    dst_sp[7] = src_sp[7] | (dst_sp_align << 9);          /* xPSR - alignment */

    /* save the current state */
    svc_cx_push_state(src_id, src_sp, dst_id);
    DEBUG_CX_SWITCH_IN();

    /* set the context stack pointer for the dst box */
    *(__uvisor_config.uvisor_box_context) = g_svc_cx_context_ptr[dst_id];

    /* switch boxes */
    vmpu_switch(src_id, dst_id);
    __set_PSP((uint32_t) dst_sp);

    /* FIXME add support for privacy (triggers register clearing) */
    return 0;
}

void UVISOR_NAKED svc_cx_switch_out(uint32_t *svc_sp)
{
    asm volatile(
        "push {lr}\n"
        "bl  __svc_cx_switch_out\n"
        "pop  {lr}\n"
        "pop  {r4-r11}\n"
        "bx   lr\n"
        :: "r" (svc_sp)
    );
}

void __svc_cx_switch_out(uint32_t *svc_sp)
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
    DEBUG_CX_SWITCH_OUT();

    /* copy return value from destination stack frame */
    src_sp[0] = dst_sp[0];
    /* the destination stack frame gets discarded automatically since the
     * corresponding stack pointer is not stored anywhere */

    /* set the context stack pointer back to the one of the src box */
    *(__uvisor_config.uvisor_box_context) = g_svc_cx_context_ptr[src_id];

    /* switch ACls */
    vmpu_switch(dst_id, src_id);
    __set_PSP((uint32_t) src_sp);
}
