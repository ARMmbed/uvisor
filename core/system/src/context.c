/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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
#include "svc.h"
#include "vmpu.h"
#include "debug.h"

/* Currently active box */
uint8_t g_active_box;

/** Stack of previous context states
 *
 * @internal
 *
 * Each element in this array holds information about a previously active
 * context. The stack grows downwards when context switches are nested, and
 * upwards when nested context switches return. */
TContextPreviousState g_context_previous_states[UVISOR_CONTEXT_MAX_DEPTH];

/** Stack pointer for the stack of previous contexts
 *
 * @internal */
uint32_t g_context_p;

/** Push the previous state to the state stack and updates the current state.
 *
 * @internal
 *
 * @warning This function trusts all the arguments that are passed to it. Input
 * verification should be performed by the caller.
 *
 * @param context_type[in]  Type of context switch to perform
 * @param src_id[in]        ID of the box we are switching context from
 * @param src_sp[in]        Stack pointer of the box we are switching from */
static void context_state_push(TContextSwitchType context_type, uint8_t src_id, uint32_t src_sp)
{
    /* Check that the state stack does not overflow. */
    if (g_context_p >= UVISOR_CONTEXT_MAX_DEPTH) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Context state stack overflow");
    }

    /* Push the source box state to the state stack. */
    g_context_previous_states[g_context_p].type = context_type;
    g_context_previous_states[g_context_p].src_id = src_id;
    g_context_previous_states[g_context_p].src_sp = src_sp;
    ++g_context_p;

    /* Update the current state of the source box. */
    g_context_current_states[src_id].sp = src_sp;
}

/** Pop the previous state from the state stack.
 *
 * @internal
 *
 * @warning This function trusts all the arguments that are passed to it. Input
 * verification should be performed by the caller.
 *
 * @returns the pointer to the previous box context state. */
static TContextPreviousState * context_state_pop(void)
{
    /* Check that the state stack does not underflow. */
    if (!g_context_p) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Context state stack underflow");
    }

    /* Pop the source box state and return it to the caller. */
    --g_context_p;
    return &g_context_previous_states[g_context_p];
}

/* Return a pointer to the previous box context state. */
TContextPreviousState * context_state_previous(void)
{
    if (!g_context_p) {
        /* There is no previous state */
        return NULL;
    } else {
        return &g_context_previous_states[g_context_p - 1];
    }
}

/* Forge a new exception stack frame and copy arguments from an old one. */
uint32_t context_forge_exc_sf(uint32_t src_sp, uint8_t dst_id, uint32_t dst_fn, uint32_t dst_lr, uint32_t xpsr, int nargs)
{
    uint8_t src_id;
    uint32_t dst_sp;
    uint32_t exc_sf_alignment_words;

    /* Source box: Gather information from the current state. */
    src_id = g_active_box;

    /* Destination box: Gather information from the current state. */
    /* Note: Here we allow source and destination box IDs to be the same, as
     * this function is used also for IRQs. */
    if (src_id == dst_id) {
        dst_sp = src_sp;
    } else {
        dst_sp = g_context_current_states[dst_id].sp;
    }

    /* Forge an exception stack frame in the destination box stack. */
    exc_sf_alignment_words = (dst_sp & 0x4) ? 1 : 0;
    dst_sp -= (CONTEXT_SWITCH_EXC_SF_BYTES + exc_sf_alignment_words * sizeof(uint32_t));

    /* Populate the new exception stack frame. */

    /* r0 - r3. Unused slots are explicitly not set. The values that will be
     * unstacked belong to the destination stack so are not leaked from another
     * context. */
    memcpy((void *) dst_sp, (void *) src_sp, sizeof(uint32_t) * nargs);

    /* r12 */
    ((uint32_t *) dst_sp)[4] = 0;

    /* lr */
    ((uint32_t *) dst_sp)[5] = dst_lr | 1;

    /* Return value. This is always OR-ed with 1 (Thumb bit). */
    ((uint32_t *) dst_sp)[6] = dst_fn | 1;

    /* Stacked xPSR register. This always includes information on the stack
     * frame alignment, regardless of the input from the caller. */
    ((uint32_t *) dst_sp)[7] = xpsr | (exc_sf_alignment_words << 9);

    /* Return stack pointer of the newly created stack frame. */
    return dst_sp;
}

/* Discard an unused exception stack frame from the destination box. */
void context_discard_exc_sf(uint8_t dst_id, uint32_t dst_sp)
{
    uint32_t exc_sf_alignment_words;

    exc_sf_alignment_words = (((uint32_t *) dst_sp)[7] & 0x4) ? 1 : 0;
    g_context_current_states[dst_id].sp = dst_sp + CONTEXT_SWITCH_EXC_SF_BYTES +
                                          exc_sf_alignment_words * sizeof(uint32_t);
}

/** Validate an exception stack frame, checking that the currently active box
 *  has access to it.
 *
 * @internal
 *
 * The exception stack frame is validated through direct access. If the stack
 * frame starting from the provided pointer is not fully accessible in
 * unprivileged mode, it means that the currently active box does not fully own
 * that memory section. This results in an access fault. The check covers the
 * following addresses:
 *  pointer[0] to pointer[7] if the stack frame is aligned
 *  pointer[0] to pointer[8] if the stack is not aligned */
uint32_t context_validate_exc_sf(uint32_t exc_sp)
{
    int i;

    /* Check the regular exception stack frame. */
    for (i = 0; i < CONTEXT_SWITCH_EXC_SF_WORDS; i++) {
        vmpu_unpriv_uint32_read(exc_sp + sizeof(uint32_t) * i);
    }

    /* Read one additional word if the stack frame is not aligned. */
    if (exc_sp & 0x4) {
        vmpu_unpriv_uint32_read(exc_sp + CONTEXT_SWITCH_EXC_SF_BYTES);
    }

    return exc_sp;
}

/* Switch the context from the source box to the destination one, using the
 * stack pointers provided as input. */
void context_switch_in(TContextSwitchType context_type, uint8_t dst_id, uint32_t src_sp, uint32_t dst_sp)
{
    uint8_t src_id;

    /* Source box: Gather information from the current state. */
    src_id = g_active_box;

    /* The source/destination box IDs can be the same (for example, in IRQs). */
    if (src_id != dst_id) {
        /* Update the context pointer to the one of the destination box. */
        *(__uvisor_config.uvisor_box_context) = (uint32_t *) g_context_current_states[dst_id].bss;

        /* Update the ID of the currently active box. */
        g_active_box = dst_id;
        UvisorBoxIndex * index = (UvisorBoxIndex *) *(__uvisor_config.uvisor_box_context);
        index->box_id_self = dst_id;

        /* Switch MPU configurations. */
        /* This function halts if it finds an error. */
        vmpu_switch(src_id, dst_id);
    }

    /* Push the state of the source box and set the stack pointer for the
     * destination box.
     * This is only needed if the context switch is tied to a function. Unbound
     * context switches require the host OS to set the correct stack pointer
     * before handling execution to the unprivileged code, and for the same
     * reason do not require state-keeping.  */
    /* This function halts if it finds an error. */
    if (context_type == CONTEXT_SWITCH_FUNCTION_GATEWAY ||
        context_type == CONTEXT_SWITCH_FUNCTION_ISR     ||
        context_type == CONTEXT_SWITCH_FUNCTION_DEBUG) {
        context_state_push(context_type, src_id, src_sp);
        __set_PSP(dst_sp);
    }
}

/** Switch the context back from the destination box to the source one.
 *
 * @internal
 *
 * In this function we keep the same naming convention of the switch-in. Hence,
 * here the destination box is the one we are leaving, the source box is the one
 * we are switching to. We do not need any input from the caller as we already
 * know where we are switching to from the stacked state.
 *
 * @warning With thread context switches there is no context switch-out, but
 * only a context switch-in (from the current thread to the next one), so this
 * function should not be used for that purpose. An error will be thrown if used
 * for thread switches. */
TContextPreviousState * context_switch_out(TContextSwitchType context_type)
{
    uint8_t src_id, dst_id;
    uint32_t src_sp;
    TContextPreviousState * previous_state;

    /* This function is not needed for unbound context switches.
     * In those cases there is only a switch from a source box to a destination
     * box, and it can be done without state keeping. It is the host OS that
     * takes care of switching the stacks. */
    if (context_type == CONTEXT_SWITCH_UNBOUND_THREAD) {
        HALT_ERROR(NOT_ALLOWED, "Unbound context switching (e.g. for thread context switching) does not need to switch "
                                "out. Just call the context_switch_in(...) function repeatedly to switch from one task "
                                "to another.");
    }

    /* Destination box: Gather information from the current state. */
    dst_id = g_active_box;

    /* Source box: Gather information from the previous state. */
    /* This function halts if it finds an error. */
    previous_state = context_state_pop();
    src_id = previous_state->src_id;
    src_sp = previous_state->src_sp;

    /* The source/destination box IDs can be the same (for example, in IRQs). */
    if (src_id != dst_id) {
        /* Update the ID of the currently active box. */
        g_active_box = src_id;

        /* Update the context pointer to the one of the source box. */
        *(__uvisor_config.uvisor_box_context) = (uint32_t *) g_context_current_states[src_id].bss;

        /* Switch MPU configurations. */
        /* This function halts if it finds an error. */
        vmpu_switch(dst_id, src_id);
    }

    /* Set the stack pointer for the source box. This is only needed if the
     * context switch is tied to a function.
     * Unbound context switches require the host OS to set the correct stack
     * pointer before handling execution to the unprivileged code. */
    if (context_type == CONTEXT_SWITCH_FUNCTION_GATEWAY ||
        context_type == CONTEXT_SWITCH_FUNCTION_ISR     ||
        context_type == CONTEXT_SWITCH_FUNCTION_DEBUG) {
        __set_PSP(src_sp);
    }

    return previous_state;
}
