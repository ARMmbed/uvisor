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
#include "context.h"
#include "box_init.h"
#include "exc_return.h"
#include "ipc.h"

void box_init(uint8_t box_id, UvisorBoxConfig const * box_cfgtbl)
{
    /* Create initial exception stack frame in box so we can switch into it
     * the very first time. */
    TContextCurrentState * state = &g_context_current_states[box_id];
    state->remaining_ms = 100;

    /* Initialize IPC for the box. */
    ipc_box_init(box_id);

    /* Set the initial state for a box. Box 0 doesn't need its initial
     * state set here, as box 0 is already running. */
    if (box_id == 0) {
        // No more work required for box 0 intialization. Return. */
        return;
    }

    // TODO Make the box_main explicit in the box config.
    uvisor_box_main_t * box_main = (uvisor_box_main_t *) box_cfgtbl->lib_config;

    state->msplim = state->sp - box_cfgtbl->stack_size;

    /* The stack pointer is currently set to exactly the upper address
     * in box memory. We can't actually store things there, so
     * decrement the stack pointer by 8 (keeping the stack pointer
     * 8-byte aligned). */
    state->sp -= 8;

    state->sp = context_forge_initial_frame(state->sp, box_main->function);
    state->msp = state->sp;

    /* On v7-M, return to threaded mode, with MSP as stack pointer. */
    /* On v8-M, return to NS threaded mode, with MSP as stack pointer. */
    state->saved_on_stack.lr = 0xFFFFFFFD;
    state->saved_on_stack.lr &= ~EXC_RETURN_SPSEL_Msk; /* Use MSP */
    state->saved_on_stack.lr &= ~EXC_RETURN_S_Msk; /* Use NS. This does nothing on ARMv7-M. */

    /* The stack must be 8-byte aligned after an exception (see ARM DDI
     * 0553A.a, paragraph RKQFB). */
    assert((state->sp & 0x7) == 0);
}
