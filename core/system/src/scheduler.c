/*
 * Copyright (c) 2017, ARM Limited, All Rights Reserved
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

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)

#include <uvisor.h>
#include "debug.h"
#include "halt.h"
#include "context.h"
#include "vmpu.h"


#if  (__ARM_FEATURE_CMSE == 3U)
/**
  \brief   Get Stack Pointer (non-secure)
  \details Returns the current value of the non-secure Stack Pointer (SP) when in secure state.
  \return               SP Register value
 */
__attribute__((always_inline)) __STATIC_INLINE uint32_t __TZ_get_SP_NS(void)
{
  register uint32_t result;

  __ASM volatile ("MRS %0, sp_ns"  : "=r" (result) );
  return(result);
}

/**
  \brief   Set Stack Pointer (non-secure)
  \details Writes the given value to the non-secure Stack Pointer (SP) when in secure state.
  \param [in]    sp  Stack Pointer Register value to set
 */
__attribute__((always_inline)) __STATIC_INLINE void __TZ_set_SP_NS(uint32_t sp)
{
  __ASM volatile ("MSR sp_ns, %0" : : "r" (sp) : "memory");
}
#endif

/* The box to switch to when the current one runs out of time. */
static int g_next_box_id = 1;

/* Set the desired time slice. */
static const int32_t time_slice_ms = 100;

/* 100 ms time slice is 2,500,000 ticks on 25MHz the fast model. */
static const uint32_t time_slice_ticks = 2500000;

/* Stacked by us in SysTick_IRQn_Handler */
typedef struct saved_reg {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t lr;
} UVISOR_PACKED saved_reg_t;

/* Stacked by architecture */
typedef struct exception_frame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t retaddr;
    uint32_t retpsr;
} UVISOR_PACKED exception_frame_t;


/* Return to the destination box. Return the LR that should be used to enter
 * the destination box via `reg->lr`. */
static void dispatch(int dst_box_id, int src_box_id, saved_reg_t * reg)
{
    DPRINTF("Switch from box ID %d to box ID %d\n", src_box_id, dst_box_id);

    /* We could be coming from unpriv or priv, secure or non-secure. We need to
     * save the stack pointer of the pre-empted process so that we can switch
     * back to it later. We need to save the EXC_RETURN and a bunch of other
     * registers as well. */

    // TODO copy state with memcpy by sharing the struct type in both places
    // (put saved_reg_t into TContextCurrentState)
    TContextCurrentState * src_state = &g_context_current_states[src_box_id];
    src_state->r4 = reg->r4;
    src_state->r5 = reg->r5;
    src_state->r6 = reg->r6;
    src_state->r7 = reg->r7;
    src_state->r8 = reg->r8;
    src_state->r9 = reg->r9;
    src_state->r10 = reg->r10;
    src_state->r11 = reg->r11;
    src_state->lr = reg->lr;
    src_state->sp = __TZ_get_SP_NS();
    src_state->psp = __TZ_get_PSP_NS();
    src_state->msp = __TZ_get_MSP_NS();
    src_state->psplim = __TZ_get_PSPLIM_NS();
    src_state->msplim = __TZ_get_MSPLIM_NS();
    src_state->control = __TZ_get_CONTROL_NS();
    src_state->basepri = __TZ_get_BASEPRI_NS();
    src_state->faultmask = __TZ_get_FAULTMASK_NS();
    src_state->primask = __TZ_get_PRIMASK_NS();

    /* This does not modify any stack pointers. It only updates uVisor internal
     * state (g_active_box, etc.) */
    const uint32_t src_sp = 0xDEADBEEF;
    const uint32_t dst_sp = 0xDEADBEE2; // Unused by context_switch_in
    context_switch_in(CONTEXT_SWITCH_UNBOUND_THREAD, dst_box_id, src_sp, dst_sp);

    /* Restore state */
    TContextCurrentState * dst_state = &g_context_current_states[dst_box_id];
    reg->r4 = dst_state->r4;
    reg->r5 = dst_state->r5;
    reg->r6 = dst_state->r6;
    reg->r7 = dst_state->r7;
    reg->r8 = dst_state->r8;
    reg->r9 = dst_state->r9;
    reg->r10 = dst_state->r10;
    reg->r11 = dst_state->r11;
    reg->lr = dst_state->lr;
    __TZ_set_PSP_NS(dst_state->psp);
    __TZ_set_MSP_NS(dst_state->msp);
    __TZ_set_PSPLIM_NS(dst_state->psplim);
    __TZ_set_MSPLIM_NS(dst_state->msplim);
    __TZ_set_CONTROL_NS(dst_state->control);
    __TZ_set_BASEPRI_NS(dst_state->basepri);
    __TZ_set_FAULTMASK_NS(dst_state->faultmask);
    __TZ_set_PRIMASK_NS(dst_state->primask);
}

/* Handle a delta time elapsed. Typically called from a timer ISR. */
static void scheduler_delta_add(uint32_t delta_ms, saved_reg_t * reg)
{
    /* If the current box has exceeded its time limit, switch to the next box
     * */
    int src_box_id = g_active_box;

    g_context_current_states[src_box_id].remaining_ms -= delta_ms;
    if (g_context_current_states[src_box_id].remaining_ms <= 0) {
        int dst_box_id = g_next_box_id;
        g_next_box_id = (g_next_box_id + 1) % g_vmpu_box_count; /* Cycle through the boxes. */

        dispatch(dst_box_id, src_box_id, reg);
        g_context_current_states[src_box_id].remaining_ms = time_slice_ms;
    }
}

void scheduler_tick(saved_reg_t * reg)
{
    scheduler_delta_add(time_slice_ms, reg);
}

void scheduler_start()
{
    /* Set up a periodic interrupt. */
    /* TODO calculate the closest tick value to a configurable target time
     * slice. For now, we are hard-coding the configuration based on the
     * known-ahead-of-time clocks in the fast model. */
    SysTick->CTRL = 0;
    SysTick->LOAD = time_slice_ticks - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

#else

/* The scheduler doesn't do anything except on v8-M. */
void scheduler_start(void)
{
}

#endif
