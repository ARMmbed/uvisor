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

    TContextCurrentState * src_state = &g_context_current_states[src_box_id];
    src_from_s = EXC_FROM_S(reg->lr);

    /* If we came from the secure side, we have to copy the register values
     * saved by HW on MSP_S into the struct so we can later restore them.
     * The registers pushed to the stack by the SW within
     * SysTick_IRQn_Handler must be always saved. */

    uint32_t bytes_to_copy = src_from_s ?
        sizeof(saved_reg_t) :
        sizeof(saved_reg_t) - sizeof(exception_frame_t);

    /* Copy the register values saved on stack. */
    memcpy((void *)&src_state->saved_on_stack, (void *)reg, bytes_to_copy);

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
    dst_from_s = EXC_FROM_S(dst_state->saved_on_stack.lr);

    /* If we are going to the secure side, we have to restore all the register
     * values from the struct, otherwise only the registers saved by the SW
     * within SysTick_IRQn_Handler must be restored. */
    bytes_to_copy = dst_from_s ?
        sizeof(saved_reg_t) :
        sizeof(saved_reg_t) - sizeof(exception_frame_t);

    /* Restore the register values saved on stack. */
    memcpy((void *)reg, (void *)&dst_state->saved_on_stack, bytes_to_copy);

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
