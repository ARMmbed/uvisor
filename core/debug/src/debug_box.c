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

void debug_print(const uint8_t * message_buffer, uint32_t size)
{
    /* Abort printing if debug box wasn't initialized yet. */
    if (!g_debug_box.initialized) {
        return;
    }

    /* Debug print relies on "deprivilege and return" flow that will currently
     * only work if:
     * - executed with exceptions enabled
     * - executed from within the exception context, i.e. when IPSR isn't 0
     * - the current exception priority is lower than of Hard Fault
     * These limitation are dictated by the fact that SVC is used for
     * deprivileging and in order for it to work SVC must be executed with
     * exceptions enabled and its priority must be higher than of the current
     * exception. SVC's priority is set to be higher than of any other
     * exception with programmable priority but this still can't beat NMI and
     * Hard Fault.
     */

    /* Make sure we're within an exception and exceptions are enabled. */
    int exception = (int)(__get_IPSR() & 0x1FF);
    if (!exception || __get_PRIMASK() || __get_FAULTMASK())    {
        return;
    }

    /* Bring exception number to CMSIS format and check exception's priority. */
    exception -= NVIC_OFFSET;
    if (exception == NonMaskableInt_IRQn || exception == HardFault_IRQn) {
        return;
    }

    /* Place the message on the interrupt stack allocating the size rounded up
     * to the next value divisible by 8. This way the stack stays aligned on
     * double-word boundary.
     * The stack memory is managed by uVisor itself and is supposed to be
     * accessible by debug box, so there's no need for the access check. */
    uint32_t sp = g_debug_interrupt_sp[g_debug_box.box_id];
    g_debug_interrupt_sp[g_debug_box.box_id] -= ((size + 7) / 8) * 8;
    void *message = (void *)g_debug_interrupt_sp[g_debug_box.box_id];
    memmove(message, message_buffer, size);

    /* Call debug driver's print function. */
    debug_deprivilege_and_return(g_debug_box.driver->debug_print, debug_return, (uint32_t)message, 0, 0, 0);

    /* Pop the message off the stack. */
    g_debug_interrupt_sp[g_debug_box.box_id] = sp;
}

void debug_halt_error(THaltError reason, const THaltInfo *halt_info)
{
    static int debugged_once_before = 0;
    void *info = NULL;

    /* If the debug box does not exist (or it has not been initialized yet), or
     * the debug box was already called once, just loop forever. */
    if (!g_debug_box.initialized || debugged_once_before) {
        uvisor_noreturn();
    } else {
        /* Remember that debug_deprivilege_and_die() has been called once. */
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
        debug_deprivilege_and_die(g_debug_box.driver->halt_error, debug_die, reason, (uint32_t)info, 0, 0);
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

/* Jump to unprivileged thread mode, the stack frame is already prepared on PSP. */
void UVISOR_NAKED debug_uvisor_deprivilege(uint32_t svc_sp, uint32_t svc_pc)
{
    /* Return to thread mode with PSP using a basic stack frame. */
    asm volatile(
        "ldr lr, =0xfffffffd\n"
        "bx lr\n"
    );
}

/* Restore the execution right after the point of deprivilege, the needed stack frame
 * was created on MSP by SVC used for deprivileging. */
void UVISOR_NAKED debug_uvisor_return(uint32_t svc_sp, uint32_t svc_pc)
{
    /* Return to handler mode with MSP using a basic stack frame. */
    asm volatile(
        "ldr lr, =0xfffffff1\n"
        "bx lr\n"
    );
}
