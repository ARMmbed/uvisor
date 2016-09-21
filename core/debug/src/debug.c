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
#include "halt.h"
#include "memory_map.h"
#include "svc.h"
#include "vmpu.h"

#ifndef DEBUG_MAX_BUFFER
#define DEBUG_MAX_BUFFER 128
#endif/*DEBUG_MAX_BUFFER*/

uint8_t g_buffer[DEBUG_MAX_BUFFER];
int g_buffer_pos;

#ifndef CHANNEL_DEBUG
void default_putc(uint8_t data)
{
    if(g_buffer_pos<(DEBUG_MAX_BUFFER-1))
        g_buffer[g_buffer_pos++] = data;
    else
        data = '\n';

    if(data == '\n')
    {
        g_buffer[g_buffer_pos] = 0;
        asm(
            "mov r0, #4\n"
            "mov r1, %[data]\n"
            "bkpt #0xAB\n"
            :
            : [data] "r" (&g_buffer)
            : "r0", "r1"
        );
        g_buffer_pos = 0;
    }
}
#endif/*CHANNEL_DEBUG*/

void debug_init(void)
{
    /* debugging bus faults requires them to be precise, so write buffering is
     * disabled; note: it slows down execution */
    SCnSCB->ACTLR |= 0x2;
}

void debug_exception_frame(uint32_t lr, uint32_t sp)
{
    int i;
    int mode = lr & 0x4;

    char exc_sf_verbose[CONTEXT_SWITCH_EXC_SF_WORDS + 1][6] = {
        "r0", "r1", "r2", "r3", "r12",
        "lr", "pc", "xPSR", "align"
    };

    dprintf("* EXCEPTION STACK FRAME\n");
    dprintf("  Exception from %s code\n", mode ? "unprivileged" : "privileged");
    dprintf("    %csp:     0x%08X\n\r", mode ? 'p' : 'm', sp);
    dprintf("    lr:      0x%08X\n\r", lr);

    /* print exception stack frame */
    dprintf("  Exception stack frame:\n");
    i = CONTEXT_SWITCH_EXC_SF_WORDS;
    if(((uint32_t *) sp)[8] & (1 << 9))
    {
        dprintf("    %csp[%02d]: 0x%08X | %s\n", mode ? 'p' : 'm', i, ((uint32_t *) sp)[i], exc_sf_verbose[i]);
    }
    for(i = CONTEXT_SWITCH_EXC_SF_WORDS - 1; i >= 0; --i)
    {
        dprintf("    %csp[%02d]: 0x%08X | %s\n", mode ? 'p' : 'm', i, ((uint32_t *) sp)[i], exc_sf_verbose[i]);
    }
    dprintf("\n");
}

void UVISOR_WEAK debug_fault_memmanage(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* MMFAR : 0x%08X\n\r\n\r", SCB->MMFAR);
}

void UVISOR_WEAK debug_fault_bus(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* BFAR  : 0x%08X\n\r\n\r", SCB->BFAR);
}

void UVISOR_WEAK debug_fault_usage(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
}

void UVISOR_WEAK debug_fault_hard(void)
{
    dprintf("* HFSR  : 0x%08X\n\r\n\r", SCB->HFSR);
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* DFSR  : 0x%08X\n\r\n\r", SCB->DFSR);
    dprintf("* BFAR  : 0x%08X\n\r\n\r", SCB->BFAR);
    dprintf("* MMFAR : 0x%08X\n\r\n\r", SCB->MMFAR);
}

void UVISOR_WEAK debug_fault_debug(void)
{
    dprintf("* DFSR  : 0x%08X\n\r\n\r", SCB->DFSR);
}

void debug_fault(THaltError reason, uint32_t lr, uint32_t sp)
{
    /* fault-specific code */
    switch(reason)
    {
        case FAULT_MEMMANAGE:
            DEBUG_PRINT_HEAD("MEMMANAGE FAULT");
            debug_fault_memmanage();
            break;
        case FAULT_BUS:
            DEBUG_PRINT_HEAD("BUS FAULT");
            debug_fault_bus();
            break;
        case FAULT_USAGE:
            DEBUG_PRINT_HEAD("USAGE FAULT");
            debug_fault_usage();
            break;
        case FAULT_HARD:
            DEBUG_PRINT_HEAD("HARD FAULT");
            debug_fault_hard();
            break;
        case FAULT_DEBUG:
            DEBUG_PRINT_HEAD("DEBUG FAULT");
            debug_fault_debug();
            break;
        default:
            DEBUG_PRINT_HEAD("[unknown fault]");
            break;
    }

    /* blue screen */
    debug_exception_frame(lr, sp);
    debug_mpu_config();
    DEBUG_PRINT_END();
}

static void debug_die(void)
{
    UVISOR_SVC(UVISOR_SVC_ID_HALT_USER_ERR, "", DEBUG_BOX_HALT);
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
     *       alignment. */
    uint32_t src_sp = context_validate_exc_sf(__get_PSP());

    /* Destination box: The debug box. */
    uint8_t dst_id = g_debug_box.box_id;

    /* Copy the xPSR from the source exception stack frame. */
    uint32_t xpsr = ((uint32_t *) src_sp)[7];

    /* Destination box: Forge the destination stack frame. */
    /* Note: We manually have to set the 4 parameters on the destination stack,
     *       so we will set the API to have nargs=0. */
    uint32_t dst_sp = context_forge_exc_sf(src_sp, dst_id, (uint32_t) debug_handler, (uint32_t) return_handler, xpsr, 0);
    ((uint32_t *) dst_sp)[0] = a0;
    ((uint32_t *) dst_sp)[1] = a1;
    ((uint32_t *) dst_sp)[2] = a2;
    ((uint32_t *) dst_sp)[3] = a3;

    /* Suspend the OS. */
    g_priv_sys_hooks.priv_os_suspend();

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
        /* Remember that debug_deprivilege_and_return() has been called once.
         * We'll reboot after the debug handler is run, so this will go back to
         * zero after the reboot. */
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
