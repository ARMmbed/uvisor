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
uint8_t g_active_box = UVISOR_BOX_ID_INVALID;

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
    if (0 == g_context_p) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Context state stack underflow");
    }

    /* Pop the source box state and return it to the caller. */
    --g_context_p;
    return &g_context_previous_states[g_context_p];
}

/* Return a pointer to the previous box context state. */
TContextPreviousState * context_state_previous(void)
{
    if (0 == g_context_p) {
        /* There is no previous state */
        return NULL;
    } else {
        return &g_context_previous_states[g_context_p - 1];
    }
}

/* Forge a new exception stack frame and copy arguments from an old one. */
uint32_t context_forge_exc_sf(uint32_t src_sp, uint8_t dst_id, uint32_t dst_fn, uint32_t dst_lr, uint32_t xpsr, int nargs)
{
    uint32_t dst_sp;
    uint32_t exc_sf_alignment_words;

    /* Destination box: Gather information from the current state. */
    /* FIXME: This prevents IRQ interrupt nesting. */
    dst_sp = g_context_current_states[dst_id].sp;

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
    /* The source box is the currently active box. */
    uint8_t src_id = g_active_box;
    if (!vmpu_is_box_id_valid(src_id)) {
        /* Note: We accept that the source box ID is invalid if this is the very
         *       first context switch. */
        if (context_type == CONTEXT_SWITCH_UNBOUND_FIRST) {
            src_id = dst_id;
        } else {
            HALT_ERROR(SANITY_CHECK_FAILED, "Context switch: The source box ID is out of range (%u).\n", src_id);
        }
    }
    if (!vmpu_is_box_id_valid(dst_id)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Context switch: The destination box ID is out of range (%u).\n", dst_id);
    }

    /* The source/destination box IDs can be the same (for example, in IRQs). */
    if (src_id != dst_id || context_type == CONTEXT_SWITCH_UNBOUND_FIRST) {
        /* Store outgoing newlib reent pointer. */
        UvisorBoxIndex * index = (UvisorBoxIndex *) g_context_current_states[src_id].bss;
        index->bss.address_of.newlib_reent = (uint32_t) *(__uvisor_config.newlib_impure_ptr);

        /* Update the context pointer to the one of the destination box. */
        index = (UvisorBoxIndex *) g_context_current_states[dst_id].bss;
        *(__uvisor_config.uvisor_box_context) = (uint32_t *) index;

        /* Update the ID of the currently active box. */
        g_active_box = dst_id;
        index->box_id_self = dst_id;

#if defined(ARCH_CORE_ARMv8M)
        /* Switch vIRQ configurations. */
        virq_switch(src_id, dst_id);
#endif /* defined(ARCH_CORE_ARMv8M) */

        /* Switch MPU configurations. */
        /* This function halts if it finds an error. */
        vmpu_switch(src_id, dst_id);

        /* Restore incoming newlib reent pointer. */
        *(__uvisor_config.newlib_impure_ptr) = (uint32_t *) index->bss.address_of.newlib_reent;
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
#if defined(ARCH_CORE_ARMv8M)
        /* FIXME: Set the right LR value depending on which NS SP is actually used. */
        __TZ_set_MSP_NS(dst_sp);
        __TZ_set_PSP_NS(dst_sp);
#else
        __set_PSP(dst_sp);
#endif
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
        /* Store outgoing newlib reent pointer. */
        UvisorBoxIndex * index = (UvisorBoxIndex *) g_context_current_states[dst_id].bss;
        index->bss.address_of.newlib_reent = (uint32_t) *(__uvisor_config.newlib_impure_ptr);

        /* Update the ID of the currently active box. */
        g_active_box = src_id;
        /* Update the context pointer to the one of the source box. */
        index = (UvisorBoxIndex *) g_context_current_states[src_id].bss;
        *(__uvisor_config.uvisor_box_context) = (uint32_t *) index;

        /* Switch MPU configurations. */
        /* This function halts if it finds an error. */
        vmpu_switch(dst_id, src_id);

        /* Restore incoming newlib reent pointer. */
        *(__uvisor_config.newlib_impure_ptr) = (uint32_t *) index->bss.address_of.newlib_reent;
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

uint32_t context_forge_initial_frame(uint32_t sp, void (*function)(const void *))
{
    /* Compute the secure alias of the NS SP */
    sp -= sizeof(exception_frame_t);

    exception_frame_t * s_ns_sp = UVISOR_GET_S_ALIAS((exception_frame_t *) sp);

    /* Clear bottom bit of the function to allow the secure side to EXC_RETURN
     * to the function. */
    s_ns_sp->retaddr = ((uint32_t) function) & ~1UL;

    /* Set T32 bit in RETPSR to run thumb2 code. */
    s_ns_sp->retpsr = xPSR_T_Msk;

    s_ns_sp->lr = 0UL; // TODO Call a uvisor_api to
    // notify uVisor that a box exited. Then uVisor could do something like stop
    // scheduling the box.

    return sp;
}
