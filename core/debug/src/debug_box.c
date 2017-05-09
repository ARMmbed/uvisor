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
#include "memory_map.h"
#include "svc.h"
#include "vmpu.h"

static void debug_die(void)
{
#if defined(ARCH_CORE_ARMv7M)
    UVISOR_SVC(UVISOR_SVC_ID_GET(error), "", DEBUG_BOX_HALT);
#else /* defined(ARCH_CORE_ARMv7M) */
    /* FIXME: Implement debug_die for ARMv8-M. */
    while (1);
#endif /* defined(ARCH_CORE_ARMv7M) */
}

void debug_reboot(TResetReason reason)
{
    if (!g_debug_box.initialized || g_active_box != g_debug_box.box_id || reason >= __TRESETREASON_MAX) {
        halt(NOT_ALLOWED);
    }

    /* Note: Currently we do not act differently based on the reset reason. */

    /* Reboot.
     * If called from unprivileged code, NVIC_SystemReset causes a fault. */
    NVIC_SystemReset();
}

/* FIXME: Currently it is not possible to return to a regular execution flow
 *        after the execution of the debug box handler. */
static void debug_deprivilege_and_return(void * debug_handler, void * return_handler,
                                         uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    /* Source box: Get the current stack pointer. */
    /* Note: The source stack pointer is only used to assess the stack
     *       alignment and to read the xpsr. */
    uint32_t src_sp = context_validate_exc_sf(__get_PSP());

    /* Destination box: The debug box. */
    uint8_t dst_id = g_debug_box.box_id;

    /* Copy the xPSR from the source exception stack frame. */
    uint32_t xpsr = vmpu_unpriv_uint32_read((uint32_t) &((uint32_t *) src_sp)[7]);

    /* Destination box: Forge the destination stack frame. */
    /* Note: We manually have to set the 4 parameters on the destination stack,
     *       so we will set the API to have nargs=0. */
    uint32_t dst_sp = context_forge_exc_sf(src_sp, dst_id, (uint32_t) debug_handler, (uint32_t) return_handler, xpsr, 0);
    ((uint32_t *) dst_sp)[0] = a0;
    ((uint32_t *) dst_sp)[1] = a1;
    ((uint32_t *) dst_sp)[2] = a2;
    ((uint32_t *) dst_sp)[3] = a3;

#if defined(ARCH_CORE_ARMv7M)
    /* Suspend the OS. */
    g_priv_sys_hooks.priv_os_suspend();
#endif /* defined(ARCH_CORE_ARMv7M) */

    context_switch_in(CONTEXT_SWITCH_FUNCTION_DEBUG, dst_id, src_sp, dst_sp);

    /* Upon execution return debug_handler will be executed. Upon return from
     * debug_handler, return_handler will be executed. */
    return;
}

uint32_t debug_get_version(void)
{
    /* TODO: This function cannot be implemented without a mechanism for
     *       de-privilege, execute, return, and re-privilege. */
    HALT_ERROR(NOT_IMPLEMENTED, "This handler is not implemented yet. Only version 0 is supported.\n\r");
    return 0;
}

void debug_halt_error(THaltError reason)
{
    static int debugged_once_before = 0;

    /* If the debug box does not exist (or it has not been initialized yet), or
     * the debug box was already called once, just loop forever. */
    if (!g_debug_box.initialized || debugged_once_before) {
        while (1);
    } else {
        /* Remember that debug_deprivilege_and_return() has been called once. */
        debugged_once_before = 1;

        /* The following arguments are passed to the destination function:
         *   1. reason
         * Upon return from the debug handler, the system will die. */
        debug_deprivilege_and_return(g_debug_box.driver->halt_error, debug_die, reason, 0, 0, 0);
    }
}

void debug_register_driver(const TUvisorDebugDriver * const driver)
{
    int i;

    /* Check if already initialized. */
    if (g_debug_box.initialized) {
        HALT_ERROR(NOT_ALLOWED, "The debug box has already been initialized.\n\r");
    }

    /* Check the driver version. */
    /* FIXME: Currently we cannot de-privilege, execute, and return to a
     *        user-provided handler, so we are not calling the get_version()
     *        handler. The version of the driver will be assumed to be 0. */

    /* Check that the debug driver table and all its entries are in public
     * flash. */
    if (!vmpu_public_flash_addr((uint32_t) driver) ||
        !vmpu_public_flash_addr((uint32_t) driver + sizeof(TUvisorDebugDriver))) {
        HALT_ERROR(SANITY_CHECK_FAILED, "The debug box driver struct must be stored in public flash.\n\r");
    }
    if (!driver) {
        HALT_ERROR(SANITY_CHECK_FAILED, "The debug box driver cannot be initialized with a NULL pointer.\r\n");
    }
    for (i = 0; i < DEBUG_BOX_HANDLERS_NUMBER; i++) {
        if (!vmpu_public_flash_addr(*((uint32_t *) driver + i))) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Each handler in the debug box driver struct must be stored in public flash.\n\r");
        }
        if (!*((uint32_t *) driver + i)) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Handlers in the debug box driver cannot be initialized with a NULL pointer.\r\n");
        }
    }

    /* Register the debug box.
     * The caller of this function is considered the owner of the debug box. */
    g_debug_box.driver = driver;
    g_debug_box.box_id = g_active_box;
    g_debug_box.initialized = 1;
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
