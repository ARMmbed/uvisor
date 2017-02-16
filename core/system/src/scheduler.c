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

static int next_box_id = 1;

/* Set the desired time slice. */
static const int32_t time_slice_ms = 100;

/* 100 ms time slice is 2,500,000 ticks on 25MHz the fast model. */
static const uint32_t time_slice_ticks = 2500000;

/* Return to the destination box. Return the LR that should be used to enter
 * the destination box. */
static void dispatch(int dst_box_id, int src_box_id)
{
    // TODO Implement this with the context switching stuff we already have
    // in context.c.
    DPRINTF("Switch from box ID %d to box ID %d\n", src_box_id, dst_box_id);
}

/* Handle a delta time elapsed. Typically called from a timer ISR. */
static void scheduler_delta_add(uint32_t delta_ms)
{
    /* If the current box has exceeded its time limit, switch to the next box
     * */
    int src_box_id = g_active_box;

    g_context_current_states[src_box_id].remaining_ms -= delta_ms;
    if (g_context_current_states[src_box_id].remaining_ms <= 0) {
        int dst_box_id = next_box_id;
        next_box_id = (next_box_id + 1) % g_vmpu_box_count; /* Cycle through the boxes. */

        dispatch(dst_box_id, src_box_id);
        g_context_current_states[src_box_id].remaining_ms = time_slice_ms;
    }
}

uint32_t scheduler_tick(uint32_t lr, uint32_t sp_ns)
{
    scheduler_delta_add(time_slice_ms);

    return lr;
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
