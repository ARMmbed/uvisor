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

#define DEBUG_PRINT_HEAD(x) {\
    DPRINTF("\n***********************************************************\n");\
    DPRINTF("                    "x"\n");\
    DPRINTF("***********************************************************\n\n");\
}
#define DEBUG_PRINT_END()   {\
    DPRINTF("***********************************************************\n\n");\
}

/* During the porting process the implementation of this function can be
 * overwritten to provide a different mechanism for printing messages (e.g. the
 * UART port instead of semihosting). */
#define DEBUG_MAX_BUFFER 128
uint8_t g_buffer[DEBUG_MAX_BUFFER];
int g_buffer_pos;

UVISOR_WEAK void default_putc(uint8_t data)
{
    if (g_buffer_pos < (DEBUG_MAX_BUFFER - 1)) {
        g_buffer[g_buffer_pos++] = data;
    } else {
        data = '\n';
    }

    if (data == '\n') {
        g_buffer[g_buffer_pos] = 0;
        asm volatile(
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

static void debug_exception_stack_frame(uint32_t lr, uint32_t sp)
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

    /* Print the exception stack frame. */
    dprintf("  Exception stack frame:\n");
    i = CONTEXT_SWITCH_EXC_SF_WORDS;
    if (((uint32_t *) sp)[8] & (1 << 9)) {
        dprintf("    %csp[%02d]: 0x%08X | %s\n", mode ? 'p' : 'm', i, ((uint32_t *) sp)[i], exc_sf_verbose[i]);
    }
    for (i = CONTEXT_SWITCH_EXC_SF_WORDS - 1; i >= 0; --i) {
        dprintf("    %csp[%02d]: 0x%08X | %s\n", mode ? 'p' : 'm', i, ((uint32_t *) sp)[i], exc_sf_verbose[i]);
    }
    dprintf("\n");
}

/* Specific architectures/ports can provide additional debug handlers. */
UVISOR_WEAK void debug_fault_memmanage_hw(void) {}
UVISOR_WEAK void debug_fault_bus_hw(void) {}

static void debug_fault_hard(void)
{
    dprintf("* HFSR  : 0x%08X\n\r\n\r", SCB->HFSR);
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* DFSR  : 0x%08X\n\r\n\r", SCB->DFSR);
    dprintf("* BFAR  : 0x%08X\n\r\n\r", SCB->BFAR);
    dprintf("* MMFAR : 0x%08X\n\r\n\r", SCB->MMFAR);
}

static void debug_fault_memmanage(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* MMFAR : 0x%08X\n\r\n\r", SCB->MMFAR);

    /* Call the MPU-specific debug handler. */
    debug_fault_memmanage_hw();
}

static void debug_fault_bus(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
    dprintf("* BFAR  : 0x%08X\n\r\n\r", SCB->BFAR);

    /* Call the MPU-specific debug handler. */
    debug_fault_bus_hw();
}

static void debug_fault_usage(void)
{
    dprintf("* CFSR  : 0x%08X\n\r\n\r", SCB->CFSR);
}

static void debug_fault_debug(void)
{
    dprintf("* DFSR  : 0x%08X\n\r\n\r", SCB->DFSR);
}

void debug_init(void)
{
    /* Debugging bus faults requires them to be precise, so write buffering is
     * disabled when debug is enabled. */
    /* Note: This slows down execution. */
    SCnSCB->ACTLR |= 0x2;
}

void debug_fault(THaltError reason, uint32_t lr, uint32_t sp)
{
    /* Fault-specific blue screen */
    switch (reason) {
        case FAULT_HARD:
            DEBUG_PRINT_HEAD("HARD FAULT");
            debug_fault_hard();
            break;
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
        case FAULT_DEBUG:
            DEBUG_PRINT_HEAD("DEBUG FAULT");
            debug_fault_debug();
            break;
        default:
            DEBUG_PRINT_HEAD("[unknown fault]");
            break;
    }

    /* Blue screen messages shared by all faults */
    debug_exception_stack_frame(lr, sp);
    debug_mpu_config();
    DEBUG_PRINT_END();
}
