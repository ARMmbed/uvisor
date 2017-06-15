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
#include <uvisor.h>
#include "debug.h"
#include "ipc.h"
#include "exc_return.h"
#include "halt.h"
#include "context.h"
#include "vmpu.h"

/* The box to switch to when the current one runs out of time. */
static int g_next_box_id;

/* Set the desired time slice. */
static const int32_t time_slice_ms = 100;

/* 10 ms time slice is 250,000 ticks on 25MHz the fast model. */
static const uint32_t time_slice_ticks = 250000;

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

    /* Stacked by architecture if S bit set in LR. This space is always
     * available to write to as our SysTick handler reserves this space even
     * when S bit is not set in the src LR. */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t retlr;
    uint32_t retaddr;
    uint32_t retpsr;
} UVISOR_PACKED saved_reg_t;

/* Stacked by architecture */
typedef struct exception_frame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t retlr;
    uint32_t retaddr;
    uint32_t retpsr;
} UVISOR_PACKED exception_frame_t;


/* Return to the destination box. Return the LR that should be used to enter
 * the destination box via `reg->lr`. */
static void dispatch(int dst_box_id, int src_box_id, saved_reg_t * reg)
{
    bool src_from_s = false;
    bool dst_from_s = false;

    /* Deliver any IPC messages. */
    ipc_drain_queue();

    /* There are four cases to handle saving and restoring core registers from.
     *
     * 1. Coming from S side, going to NS (b9). Save information from
     *    the real secure stack frame and discard it before restoring from the
     *    NS stack frame. Restore from the NS stack frame.
     * 2. Coming from NS side, going to S (f9). Keep the real NS stack
     *    frame around and use it to restore from later. Use the forged secure
     *    stack frame to restore state.
     * 3. Coming from S side, going to S. Save information from the real secure
     *    stack frame and use the modified real secure stack frame to restore
     *    state.
     * 4. Coming from NS side, going to NS. Keep the real NS stack frame around
     *    and use it to restore from later. Don't use the allocated secure
     *    stack frame. Use the destination exception stack frame to restore
     *    state.
     */

    // TODO copy state with memcpy by sharing the struct type in both places
    // (put saved_reg_t into TContextCurrentState)
    TContextCurrentState * src_state = &g_context_current_states[src_box_id];
    src_from_s = EXC_FROM_S(reg->lr);
    /* If we came from the secure side, we have to save the register values in
     * the struct so we can later restore them. */
    if (src_from_s) {
        src_state->r0 = reg->r0;
        src_state->r1 = reg->r1;
        src_state->r2 = reg->r2;
        src_state->r3 = reg->r3;
        src_state->r12 = reg->r12;
        src_state->retlr = reg->retlr;
        src_state->retaddr = reg->retaddr;
        src_state->retpsr = reg->retpsr;
    }
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
    dst_from_s = EXC_FROM_S(dst_state->lr);
    /* If we are going to the secure side, we have to restore the register
     * values from the struct to the secure stack frame (and we always have a
     * secure stack frame, even when we don't use it-- our SysTick_IRQn_Handler
     * makes certain of this). */
    if (dst_from_s) {
        /* Populate the secure exception frame so that when we return from the
         * SysTick handler (not this function) we end up with a proper
         * exception stack frame on the secure stack and use it to restore the
         * state of r0-r3, r12, retlr, retaddr, and xpsr. */
        reg->r0 = dst_state->r0;
        reg->r1 = dst_state->r1;
        reg->r2 = dst_state->r2;
        reg->r3 = dst_state->r3;
        reg->r12 = dst_state->r12;
        reg->retlr = dst_state->retlr;
        reg->retaddr = dst_state->retaddr;
        reg->retpsr = dst_state->retpsr;
    }
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
        g_next_box_id = (g_next_box_id + 1) % g_vmpu_box_count; /* Cycle through the boxes. */
        dispatch(g_next_box_id, src_box_id, reg);
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

void UVISOR_NAKED SysTick_IRQn_Handler(void)
{
    /* When switching from NS to S via secure exception or IRQ, the NS
     * registers are not stacked. Secure side has to read all this state so
     * that it can restore it when resuming that box. We push all this state
     * onto the stack and read it from C as a struct. When restoring state, we
     * write the register values to the stack from C and then pop the
     * registers. */
    asm volatile(
        "tst lr, #0x40\n"     /* Is source frame stacked on the secure side? */
        "it eq\n"
        "subeq sp, #0x20\n"   /* No, allocate a secure stack frame. */
        "push {r4-r11, lr}\n" /* Save registers not in exception frame. */
        "mov r0, sp\n"
        "bl scheduler_tick\n"
        "pop {r4-r11, lr}\n"  /* Restore registers not in exception frame. */
        "tst lr, #0x40\n"     /* Is dest frame stacked on the secure side? */
        "it eq\n"
        "addeq sp, #0x20\n"   /* No, deallocate the secure stack frame. */
        "bx lr\n"
    );
}
