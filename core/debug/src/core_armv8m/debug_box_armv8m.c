/*
 * Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
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
#include "context.h"
#include "linker.h"
#include "secure_transitions.h"
#include "vmpu_mpu.h"
#include "vmpu_unpriv_access.h"

extern uint32_t g_debug_interrupt_sp[];

void debug_die(void)
{
    uvisor_noreturn();
}

/* Note: On ARMv8-M the return_handler is executed in S mode. */
void debug_deprivilege_and_die(void * debug_handler, void * return_handler,
                               uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    /* Destination box: The debug box. */
    uint8_t dst_id = g_debug_box.box_id;
    if (__get_IPSR()) {
        /* Source box: Get the current stack pointer. */
        /* Note: The source stack pointer is only used to assess the stack
        *       alignment and to read the xpsr. */
        uint32_t src_sp = context_validate_exc_sf(__TZ_get_SP_NS());

        /* FIXME: This makes the debug box overwrite the top of the interrupt stack! */
        g_context_current_states[dst_id].sp = g_debug_interrupt_sp[dst_id];

        /* Destination box: Forge the destination stack frame. */
        /* Note: We manually have to set the 4 parameters on the destination stack,
        *       so we will set the API to have nargs=0. */
        uint32_t dst_sp = context_forge_exc_sf(src_sp, dst_id, (uint32_t) debug_handler, (uint32_t) return_handler, xPSR_T_Msk, 0);
        ((uint32_t *) dst_sp)[0] = a0;
        ((uint32_t *) dst_sp)[1] = a1;
        ((uint32_t *) dst_sp)[2] = a2;
        ((uint32_t *) dst_sp)[3] = a3;

        /* Stop all lower-than-SVC-priority interrupts. FIXME Enable debug box to
        * do things that require interrupts. One idea would be to provide an SVC
        * to re-enable interrupts that can only be called by the debug box during
        * debug handling. */
        //__set_BASEPRI(__UVISOR_NVIC_MIN_PRIORITY << (8U - __NVIC_PRIO_BITS));

        context_switch_in(CONTEXT_SWITCH_FUNCTION_DEBUG, dst_id, src_sp, dst_sp);
    } else {
        /* Switch to the debug box.
        * We use a regular process switch, so we don't need a dedicated stack for
        * the debug box. */

        context_switch_in(CONTEXT_SWITCH_FUNCTION_DEBUG, dst_id, __TZ_get_SP_NS(), g_context_current_states[dst_id].sp);

        /* De-privilege, call the debug box handler, re-privilege, call the return
        * handler. */
        uint32_t caller = UVISOR_GET_NS_ALIAS(UVISOR_GET_NS_ADDRESS((uint32_t) debug_handler));
        SECURE_TRANSITION_S_TO_NS(caller, a0, a1, a2, a3);
        ((void (*)(void)) return_handler)();
    }
}
