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
#include "debug.h"
#include "context.h"
#include "exc_return.h"
#include "halt.h"
#include "svc.h"
#include "vmpu.h"

TDebugBox g_debug_box;

void debug_reboot(TResetReason reason)
{
    if (!g_debug_box.initialized || g_active_box != g_debug_box.box_id || reason >= __TRESETREASON_MAX) {
        halt(NOT_ALLOWED, NULL);
    }

    /* Note: Currently we do not act differently based on the reset reason. */

    /* Reboot.
     * If called from unprivileged code, NVIC_SystemReset causes a fault. */
    NVIC_SystemReset();
}

uint32_t g_debug_interrupt_sp[UVISOR_MAX_BOXES];

void debug_halt_error(THaltError reason, const THaltInfo *halt_info)
{
    static int debugged_once_before = 0;
    void *info = NULL;

    /* If the debug box does not exist (or it has not been initialized yet), or
     * the debug box was already called once, just loop forever. */
    if (!g_debug_box.initialized || debugged_once_before) {
        while (1);
    } else {
        /* Remember that debug_deprivilege_and_return() has been called once. */
        debugged_once_before = 1;

        /* Place the halt info on the interrupt stack. */
        if (halt_info) {
            g_debug_interrupt_sp[g_debug_box.box_id] -= sizeof(THaltInfo);
            info = (void *)g_debug_interrupt_sp[g_debug_box.box_id];
            memcpy(info, halt_info, sizeof(THaltInfo));
        }

        /* The following arguments are passed to the destination function:
         *   1. reason
         *   2. halt info
         * Upon return from the debug handler, the system will die. */
        debug_deprivilege_and_return(g_debug_box.driver->halt_error, debug_die, reason, (uint32_t)info, 0, 0);
    }
}

/* FIXME This is a bit platform specific. Consider moving to a platform
 * specific location. */
uint32_t debug_box_enter_from_priv(uint32_t lr) {
    uint32_t shcsr;
    uint32_t from_priv = !EXC_FROM_NP(lr);

    /* If we are not handling an exception caused from privileged mode, return
     * the original lr. */
    if (!from_priv) {
        return lr;
    }

    shcsr = SCB->SHCSR;

    /* Make sure SVC is active. */
    assert(shcsr & SCB_SHCSR_SVCALLACT_Msk);

    /* We had a fault (from SVC), so clear the SVC fault before returning. SVC
     * and all other exceptions must be no longer active after the EXC RETURN,
     * or else we cause usage faults when doing SVCs later (for example, to
     * reboot via the debug_reboot SVC). */
    SCB->SHCSR = shcsr & ~SCB_SHCSR_SVCALLACT_Msk;

    /* Return to Thread mode and use the Process Stack for return. The PSP will
     * have been changed already. */
    return 0xFFFFFFFD;
}
