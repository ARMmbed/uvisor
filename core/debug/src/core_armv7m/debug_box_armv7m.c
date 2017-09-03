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
#include "context.h"
#include "debug.h"
#include "vmpu.h"
#include "vmpu_mpu.h"

void debug_die(void)
{
    UVISOR_SVC(UVISOR_SVC_ID_GET(error), "", DEBUG_BOX_HALT);
}

extern uint32_t g_debug_interrupt_sp[];

/* FIXME: Currently it is not possible to return to a regular execution flow
 *        after the execution of the debug box handler. */
/* Note: On ARMv7-M the return_handler is executed in NP mode. */
void debug_deprivilege_and_die(void * debug_handler, void * return_handler,
                               uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    /* Source box: Get the current stack pointer. */
    /* Note: The source stack pointer is only used to assess the stack
     *       alignment and to read the xpsr. */
    uint32_t src_sp = context_validate_exc_sf(__get_PSP());

    /* Destination box: The debug box. */
    uint8_t dst_id = g_debug_box.box_id;

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

    /* Suspend the OS. */
    g_priv_sys_hooks.priv_os_suspend();

    /* Stop all lower-than-SVC-priority interrupts. FIXME Enable debug box to
     * do things that require interrupts. One idea would be to provide an SVC
     * to re-enable interrupts that can only be called by the debug box during
     * debug handling. */
    __set_BASEPRI(__UVISOR_NVIC_MIN_PRIORITY << (8U - __NVIC_PRIO_BITS));

    context_switch_in(CONTEXT_SWITCH_FUNCTION_DEBUG, dst_id, src_sp, dst_sp);

    /* Upon execution return debug_handler will be executed. Upon return from
     * debug_handler, return_handler will be executed. */
    return;
}

/* This function is called by the user to return to uvisor after deprivileging. */
void UVISOR_NAKED UVISOR_NORETURN debug_return(void)
{
    asm volatile(
        "svc %[retn]"

        ::[retn] "i" ((UVISOR_SVC_ID_RETURN) & 0xFF)
    );
}

void debug_deprivilege_and_return(void * debug_handler, void * return_handler,
                                  uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    /* We're going to switch into the debug box. */
    uint8_t dst_id = g_debug_box.box_id;

    /* Use the interrupt stack during deprivileging. */
    g_context_current_states[dst_id].sp = g_debug_interrupt_sp[dst_id];

    /* Forge the stack frame for deprivileging with up to 4 parameters to pass. */
    uint32_t dst_sp = context_forge_exc_sf(0, dst_id, (uint32_t)debug_handler, (uint32_t)return_handler, xPSR_T_Msk, 0);
    ((uint32_t *)dst_sp)[0] = a0;
    ((uint32_t *)dst_sp)[1] = a1;
    ((uint32_t *)dst_sp)[2] = a2;
    ((uint32_t *)dst_sp)[3] = a3;

    context_switch_in(CONTEXT_SWITCH_FUNCTION_DEBUG, dst_id, __get_PSP(), dst_sp);

    /* Save the current context on the stack, deprivilege and restore the context upon return. */
    asm volatile(
        /* Save general purpose registers that won't be saved by the following SVC. */
        "push {r4 - r11}\n"
        
        /* Execute SVC that will perform the deprivileging. */
        "svc %[depriv]\n"
        
        /* We'll return to this point after reprivileging.
         * At this moment a part of the context will be restored from the stack frame created by the above SVC.
         * The remaining general purpose registers will be restored now. */
        "pop {r4 - r11}\n"

        ::[depriv] "i" ((UVISOR_SVC_ID_DEPRIVILEGE) & 0xFF)
    );

    context_switch_out(CONTEXT_SWITCH_FUNCTION_DEBUG);
}
