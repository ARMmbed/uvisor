/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#include "halt.h"
#include "linker.h"
#include "vmpu.h"

/** Flag used to remember whether the box initialization has happened.
 * @internal
 */
static bool g_box_main_happened;

/** Counter of boxes that have already been initialized.
 * @internal
 */
static uint8_t g_box_main_counter;

/** Cached the stack pointer of the first SVCall in the initialization chain.
 * @internal
 */
static uint32_t g_box_main_box0_sp;

/** Thunk function for the box main initialization.
 * @internal
 * Since the box initialization is implemented as a recursive gateway that
 * iterates over all the secure boxes, this thunk is the same SVCall as the
 * actual API that the user calls from uvisor-lib. The actual SVCall handler
 * will check at which stage of the recursion we are. */
static void UVISOR_NAKED box_main_thunk(void)
{
    UVISOR_SVC(UVISOR_SVC_ID_BOX_MAIN_NEXT, "");
}

/** Initialize all boxes that require it by running the default box main
 *  handler.
 * @internal
 * This function is implemented as a wrapper, needed to make sure that the lr
 * register doesn't get polluted. The actual function is \ref
 * box_main_context_switch_next.
 */
void UVISOR_NAKED box_main_next(uint32_t src_svc_sp)
{
    /* According to the ARM ABI, r0 will have the following value when this
     * function is called:
     *   r0 = src_svc_sp */
    asm volatile(
        "push {r4 - r11}\n"                   /* Store the callee-saved registers on the MSP (privileged). */
        "push {lr}\n"                         /* Preserve the lr register. */
        "bl   box_main_context_switch_next\n" /* box_main_context_switch_next(src_svc_sp) */
        "pop  {pc}\n"                         /* Return. Note: Callee-saved registers are not popped here. */
                                              /* The box main handler will be executed after this. */
        :: "r" (src_svc_sp)
    );
}

/** Perform a chain of context switches to initialize all secure boxes.
 * @internal
 * This function implements a recursive chain of context switches, where all
 * destination contexts execute the same default handler, registers in flash.
 * @note This function can only be run once.
 * @note Only boxes different from box 0 can be initialized. If an
 * initialization handler is provided for box 0 it will be ignored.
 * @param src_svc_sp[in]    Unprivileged stack pointer at the time of the SVCall
 */
void box_main_context_switch_next(uint32_t src_svc_sp)
{
    /* Check if this is the first time the box initialization procedure is
     * requested. */
    if (g_box_main_happened) {
        HALT_ERROR(NOT_ALLOWED, "The box initialization routine can only be executed once.");
    }

    /* If there is only one box (which is box 0) return immediately. Box 0 is
     * not allowed to have a box initialization function. */
    if (g_vmpu_box_count == 1) {
        g_box_main_happened = true;
        return;
    }

    /* If the currently active box is not the one we expected from our running
     * counter, then the recursive chain of context switches has been broken. */
    if (g_active_box != g_box_main_counter) {
        HALT_ERROR(NOT_ALLOWED, "The chain of box initializations has been broken.");
    }

    /* Get the handler from the table of exported handlers. */
    /* Note: We assume this table has already been verified. */
    uint32_t const dst_fn = (uint32_t) __uvisor_config.box_main_handler;
    if (dst_fn == 0) {
        HALT_ERROR(NOT_ALLOWED, "The box initialization procedure requires an unprivileged handler to be specified.");
    }

    /* Find the next box to switch to, if any.
     * The box needs to have an initialization function defined, otherwise we
     * skip it. */
    uint8_t dst_id = g_box_main_counter + 1;
    UvisorBoxConfig const * * box_cfg = (UvisorBoxConfig const * *) __uvisor_config.cfgtbl_ptr_start;
    while (dst_id < g_vmpu_box_count) {
        if (box_cfg[dst_id]->main_function != NULL) {
            break;
        } else {
            dst_id++;
        }
    }

    /* The stack pointer provided to us as an input comes from an SVC exception.
     * We must check that it corresponds to a full frame that the source box can
     * access. */
    /* This function halts if it finds an error. */
    uint32_t src_sp = context_validate_exc_sf(src_svc_sp);
    if (g_box_main_counter == 0) {
        g_box_main_box0_sp = src_sp;
    } else {
        /* If this is not the first context switch in the chain, then we first need
         * to return from the previous one. */
        context_discard_exc_sf(g_active_box, src_sp);
        context_switch_out(CONTEXT_SWITCH_FUNCTION_GATEWAY);
    }

    /* Recursion termination condition.
     * If all boxes have been initialized, we can perform the very last context
     * switch-out and return to the original caller. */
    if (dst_id >= g_vmpu_box_count) {
        g_box_main_happened = true;
        return;
    }

    /* If the code gets to this point, then we still have one or more boxes to
     * initialize. */

    /* The values of dst_id and box_cfg after the loop above represent the box
     * to initialize. */
    g_box_main_counter = dst_id;

    /* Forge a stack frame for the destination box. */
    uint32_t xpsr = ((uint32_t *) g_box_main_box0_sp)[7];
    uint32_t dst_sp = context_forge_exc_sf(g_box_main_box0_sp, dst_id, dst_fn, (uint32_t) box_main_thunk, xpsr, 0);

    /* Get the first state for the box we are switching to. This will be used to
     * pass the box stack pointer down to the initalization function. */
    TContextPreviousState * first_state = context_state_first(dst_id);

    /* Populate the 4 arguments passed down to the destination box.*/
    ((uint32_t *) dst_sp)[0] = (uint32_t) box_cfg[dst_id]->main_function; /* Box-specific initialization handler */
    ((uint32_t *) dst_sp)[1] = (uint32_t) box_cfg[dst_id]->main_priority; /* Box-specific initialization handler priority */
    ((uint32_t *) dst_sp)[2] = first_state->src_sp;                       /* Box initial stack pointer */
    ((uint32_t *) dst_sp)[3] = box_cfg[dst_id]->stack_size;               /* Box private stack size */

    /* Perform the context switch to the destination box.
     * This context switch will update the internal context state, so that the
     * destination handler can be pre-empted if needed. */
    /* This function halts if it finds an error. */
    context_switch_in(CONTEXT_SWITCH_FUNCTION_GATEWAY, dst_id, g_box_main_box0_sp, dst_sp);
}
