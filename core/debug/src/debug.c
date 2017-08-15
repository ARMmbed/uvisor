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
#include <stdbool.h>

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
#define DEBUG_SEMIHOSTING_MAGIC 0xDEADD00D

static uint8_t g_buffer[DEBUG_MAX_BUFFER];
static uint32_t g_buffer_pos = 0;

__attribute__((section(".uninitialized"))) static uint32_t g_semihosting_magic;

void debug_semihosting_enable(void)
{
    g_semihosting_magic = DEBUG_SEMIHOSTING_MAGIC;
}

UVISOR_WEAK void default_putc(uint8_t data)
{
    if (DEBUG_SEMIHOSTING_MAGIC == g_semihosting_magic) {

        g_buffer[g_buffer_pos++] = data;
        if (g_buffer_pos == (DEBUG_MAX_BUFFER - 1)) {
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
}

static void debug_exception_stack_frame(uint32_t lr, uint32_t sp)
{
    dprintf("* EXCEPTION STACK FRAME\r\n");
    dprintf("\r\n");

    /* Determine the exception origin. */
    bool from_np = EXC_FROM_NP(lr);
    bool from_psp = EXC_FROM_PSP(lr);
#if defined(ARCH_MPU_ARMv8M)
    bool from_s = EXC_FROM_S(lr);
    bool to_s = EXC_TO_S(lr);
#endif /* defined(ARCH_MPU_ARMv8M) */

    /* Debug the value of EXC_RETURN. */
    dprintf("  lr: 0x%08X\r\n", lr);
#if defined(ARCH_MPU_ARMv8M)
    dprintf("  --> Exception from %s mode, handled in %s mode.\r\n", from_s ? "S" : "NS", to_s ? "S" : "NS");
    dprintf("  --> R4-R11 %sstacked.\r\n", (lr & EXC_RETURN_DCRS_Msk) ? "" : "not ");
#endif /* defined(ARCH_MPU_ARMv8M) */
    dprintf("  --> FP stack frame %ssaved.\r\n", (lr & EXC_RETURN_FType_Msk) ? "not " : "");
    dprintf("  --> Exception from %s mode.\r\n", from_np ? "NP" : "P");
    dprintf("  --> Return to %cSP.\r\n", from_psp ? 'P' : 'M');
    dprintf("\r\n");

    /* Debug the exception stack frame. */

    /* Names of the default stack frame elements. */
    char exc_sf_verbose[CONTEXT_SWITCH_EXC_SF_WORDS + 1][6] = {
        "r0", "r1", "r2", "r3", "r12",
        "lr", "pc", "xPSR", "align"
    };
    dprintf("  sp: 0x%08X\r\n", sp);

    /* The regular exception stack frame might be preceded by an additional one
     * on ARMv8-M. */
#if defined(ARCH_MPU_ARMv8M)
    int offset = ((lr & EXC_RETURN_DCRS_Msk) ? CONTEXT_SWITCH_EXC_SF_ADDITIONAL_WORDS : 0);
#else
    int offset = 0;
#endif /* defined(ARCH_MPU_ARMv8M) */

    /* Print the alignment field, if required. */
    if (((uint32_t *) sp)[offset + CONTEXT_SWITCH_EXC_SF_WORDS - 1] & (1 << 9)) {
        dprintf("      sp[%02d]: 0x%08X | %s\r\n",
                offset + CONTEXT_SWITCH_EXC_SF_WORDS,
                ((uint32_t *) sp)[offset + CONTEXT_SWITCH_EXC_SF_WORDS],
                exc_sf_verbose[CONTEXT_SWITCH_EXC_SF_WORDS]
        );
    }

    /* Dump the regular exception stack frame. */
    int i = CONTEXT_SWITCH_EXC_SF_WORDS - 1;
    for (; i >= 0; --i) {
        dprintf("      sp[%02d]: 0x%08X | %s\r\n",
                offset + i,
                ((uint32_t *) sp)[offset + i],
                exc_sf_verbose[i]
        );
    }

    /* On ARMv8-M, dump an additional stack frame when needed. */
#if defined(ARCH_MPU_ARMv8M)
    if (lr & EXC_RETURN_DCRS_Msk) {
        char exc_sf_additional_verbose[CONTEXT_SWITCH_EXC_SF_ADDITIONAL_WORDS+ 1][6] = {
            "sign", "align",
            "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"
        };
        for (i = CONTEXT_SWITCH_EXC_SF_ADDITIONAL_WORDS - 1; i >= 0; --i) {
            dprintf("      sp[%02d]: 0x%08X | %s\n",
                    i,
                    ((uint32_t *) sp)[i],
                    exc_sf_additional_verbose[i]
            );
        }
    }
#endif /* defined(ARCH_MPU_ARMv8M) */

    dprintf("\r\n");

    dprintf("\r\n");
}

/* Specific architectures/ports can provide additional debug handlers. */
UVISOR_WEAK void debug_fault_memmanage_hw(void) {}
UVISOR_WEAK void debug_fault_bus_hw(void) {}
#if defined(ARCH_MPU_ARMv8M)
UVISOR_WEAK void debug_fault_secure_hw(void) {}
#endif /* defined(ARCH_MPU_ARMv8M) */

static void debug_fault_memmanage(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    uint32_t mmfsr = SCB->CFSR;
    dprintf("  CFSR: 0x%08X\r\n", mmfsr);
    if (mmfsr & SCB_CFSR_MMARVALID_Msk) {
        dprintf("  MMFAR: 0x%08X\r\n", SCB->MMFAR);
    } else {
        dprintf("  --> MMFAR not valid.\r\n");
    }
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    if (mmfsr & SCB_CFSR_MLSPERR_Msk) {
        dprintf("  --> MLSPERR: lazy FP state preservation.\r\n");
    }
#endif /* defined(__FPU_PRESENT) && __FPU_PRESENT == 1 */
    if (mmfsr & SCB_CFSR_MSTKERR_Msk) {
        dprintf("  --> MSTKERR: exception entry.\r\n");
    }
    if (mmfsr & SCB_CFSR_MUNSTKERR_Msk) {
        dprintf("  --> MUNSTKERR: exception exit.\r\n");
    }
    if (mmfsr & SCB_CFSR_DACCVIOL_Msk) {
        dprintf("  --> DACCVIOL: data access violation.\r\n");
    }
    if (mmfsr & SCB_CFSR_IACCVIOL_Msk) {
        dprintf("  --> IACCVIOL: precise data access.\r\n");
    }
    if (mmfsr & SCB_CFSR_IBUSERR_Msk) {
        dprintf("  --> IBUSERR: MPU fault/XN default map violation at instruction fetch (instruction has been issued).\r\n");
    }
    dprintf("\r\n");

    /* Call the MPU-specific debug handler. */
    debug_fault_memmanage_hw();
}

static void debug_fault_bus(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    uint32_t bfsr = SCB->CFSR;
    dprintf("  CFSR: 0x%08X\r\n", bfsr);
    if (bfsr & SCB_CFSR_BFARVALID_Msk) {
        dprintf("  BFAR: 0x%08X\r\n", SCB->BFAR);
    } else {
        dprintf("  --> BFAR not valid.\r\n");
    }
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    if (bfsr & SCB_CFSR_LSPERR_Msk) {
        dprintf("  --> LSPERR: lazy FP state preservation.\r\n");
    }
#endif /* defined(__FPU_PRESENT) && __FPU_PRESENT == 1 */
    if (bfsr & SCB_CFSR_STKERR_Msk) {
        dprintf("  --> STKERR: exception entry.\r\n");
    }
    if (bfsr & SCB_CFSR_UNSTKERR_Msk) {
        dprintf("  --> UNSTKERR: exception exit.\r\n");
    }
    if (bfsr & SCB_CFSR_IMPRECISERR_Msk) {
        dprintf("  --> IMPRECISERR: imprecise data access.\r\n");
    }
    if (bfsr & SCB_CFSR_PRECISERR_Msk) {
        dprintf("  --> PRECISERR: precise data access.\r\n");
    }
    if (bfsr & SCB_CFSR_IBUSERR_Msk) {
        dprintf("  --> IBUSERR: instruction prefetch (instruction has been issued).\r\n");
    }
    dprintf("\r\n");

    /* Call the MPU-specific debug handler. */
    debug_fault_bus_hw();
}

static void debug_fault_usage(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    uint32_t ufsr = SCB->CFSR;
    dprintf("  CFSR: 0x%08X\r\n", ufsr);
    if (ufsr & SCB_CFSR_DIVBYZERO_Msk) {
        dprintf("  --> DIVBYZERO: divide by zero.\r\n");
    }
    if (ufsr & SCB_CFSR_UNALIGNED_Msk) {
        dprintf("  --> UNALIGNED: unaligned access.\r\n");
    }
#if defined(ARCH_MPU_ARMv8M)
    if (ufsr & SCB_CFSR_STKOF_Msk) {
        dprintf("  --> STKOF: stack overflow.\r\n");
    }
#endif /* defined(ARCH_MPU_ARMv8M) */
    if (ufsr & SCB_CFSR_NOCP_Msk) {
        dprintf("  --> NOCP: coprocessor access (disabled/absent).\r\n");
    }
    if (ufsr & SCB_CFSR_INVPC_Msk) {
        dprintf("  --> INVPC: integrity checks on EXC_RETURN.\r\n");
    }
    if (ufsr & SCB_CFSR_INVSTATE_Msk) {
        dprintf("  --> INVSTATE: instruction executed with invalid EPSR.T/.IT field.\r\n");
    }
    if (ufsr & SCB_CFSR_UNDEFINSTR_Msk) {
        dprintf("  --> UNDEFINSTR: undefined instruction.\r\n");
    }
    dprintf("\r\n");
}

#if defined(ARCH_MPU_ARMv8M)
static void debug_fault_secure(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    uint32_t sfsr = SAU->SFSR;
    dprintf("  SFSR: 0x%08X\r\n", sfsr);
    if (sfsr & SAU_SFSR_SFARVALID_Msk) {
        dprintf("  SFAR: 0x%08X\r\n", SAU->SFAR);
    } else {
        dprintf("  --> SFAR not valid.\r\n");
    }
    if (sfsr & SAU_SFSR_LSERR_Msk) {
        dprintf("  --> LSERR: lazy state preservation.\r\n");
    }
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    if (sfsr & SAU_SFSR_LSPERR_Msk) {
        dprintf("  --> LSPERR: lazy FP state preservation.\r\n");
    }
#endif /* defined(__FPU_PRESENT) && __FPU_PRESENT == 1 */
    if (sfsr & SAU_SFSR_INVTRAN_Msk) {
        dprintf("  --> INVTRAN: invalid transition (branch not flagged for transition).\r\n");
    }
    if (sfsr & SAU_SFSR_AUVIOL_Msk) {
        dprintf("  --> AUVIOL: attribution unit violation.\r\n");
    }
    if (sfsr & SAU_SFSR_INVER_Msk) {
        dprintf("  --> INVER: invalid EXC_RETURN (S/NS state bit).\r\n");
    }
    if (sfsr & SAU_SFSR_INVIS_Msk) {
        dprintf("  --> INVIS: invalid integrity signature in exception stack frame.\r\n");
    }
    if (sfsr & SAU_SFSR_INVEP_Msk) {
        dprintf("  --> INVEP: invalid entry point NS->S (no SG or no matching SAU/IDAU region found).\r\n");
    }
    dprintf("\r\n");

    /* Call the MPU-specific debug handler. */
    debug_fault_secure_hw();
}
#endif /* defined(ARCH_MPU_ARMv8M) */

static void debug_fault_debug(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    dprintf("  DFSR  : 0x%08X\r\n\r\n", SCB->DFSR);
}

static void debug_box_id(void) {
    dprintf("* Active Box ID: %u\r\n", g_active_box);
}

static void debug_fault_hard(void)
{
    dprintf("* FAULT SYNDROME REGISTERS\r\n");
    dprintf("\r\n");

    uint32_t hfsr = SCB->HFSR;
    bool forced = false;
    dprintf("  HFSR: 0x%08X\r\n", hfsr);
    if (hfsr & SCB_HFSR_DEBUGEVT_Msk) {
        dprintf("  --> DEBUGEVT: debug event occurred.\r\n");
    }
    if (hfsr & SCB_HFSR_FORCED_Msk) {
        dprintf("  --> FORCED: another priority escalated to hard fault.\r\n");
        forced = true;
    }
    if (hfsr & SCB_HFSR_VECTTBL_Msk) {
        dprintf("  --> VECTTBL: vector table read fault on exception processing.\r\n");
    }

    if (forced) {
        uint32_t shcsr = SCB->SHCSR;
        if (shcsr & SCB_SHCSR_USGFAULTPENDED_Msk) {
            dprintf("  --> Escalated from Usage fault.\r\n");
            dprintf("\r\n");
            debug_fault_usage();
            return;
        }
        if (shcsr & SCB_SHCSR_BUSFAULTPENDED_Msk) {
            dprintf("  --> Escalated from Bus fault.\r\n");
            dprintf("\r\n");
            debug_fault_bus();
            return;
        }
        if (shcsr & SCB_SHCSR_MEMFAULTPENDED_Msk) {
            dprintf("  --> Escalated from MemManage fault.\r\n");
            dprintf("\r\n");
            debug_fault_memmanage();
            return;
        }
        if (shcsr & SCB_SHCSR_SVCALLPENDED_Msk) {
            dprintf("  --> Escalated from SVCall.\r\n");
        }
        if (((SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) >> SCB_ICSR_VECTPENDING_Pos) == (SysTick_IRQn + NVIC_OFFSET)) {
            dprintf("  --> Escalated from SysTick IRQ.\r\n");
        }
    }
    dprintf("\r\n");
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
            debug_box_id();
            debug_fault_hard();
            break;
        case FAULT_MEMMANAGE:
            DEBUG_PRINT_HEAD("MEMMANAGE FAULT");
            debug_box_id();
            debug_fault_memmanage();
            break;
        case FAULT_BUS:
            DEBUG_PRINT_HEAD("BUS FAULT");
            debug_box_id();
            debug_fault_bus();
            break;
        case FAULT_USAGE:
            DEBUG_PRINT_HEAD("USAGE FAULT");
            debug_box_id();
            debug_fault_usage();
            break;
#if defined(ARCH_MPU_ARMv8M)
        case FAULT_SECURE:
            DEBUG_PRINT_HEAD("SECURE FAULT");
            debug_box_id();
            debug_fault_secure();
            break;
#endif /* defined(ARCH_MPU_ARMv8M) */
        case FAULT_DEBUG:
            DEBUG_PRINT_HEAD("DEBUG FAULT");
            debug_box_id();
            debug_fault_debug();
            break;
        default:
            DEBUG_PRINT_HEAD("[unknown fault]");
            debug_box_id();
            break;
    }

    /* Blue screen messages shared by all faults */
    debug_exception_stack_frame(lr, sp);
    debug_mpu_config();
#if defined(ARCH_MPU_ARMv8M)
    debug_sau_config();
#endif /* defined(ARCH_MPU_ARMv8M) */
    DEBUG_PRINT_END();
}

bool debug_collect_halt_info(uint32_t lr, uint32_t sp, THaltInfo *halt_info)
{
    /* For security reasons we don't want to print uVisor faults, since it
     * could expose secrets. To debug uVisor faults users will have to use
     * a uVisor debug build with semihosting. */
#if defined(ARCH_MPU_ARMv8M)
    bool no_halt_info = EXC_FROM_S(lr);
#else
    bool no_halt_info = !EXC_FROM_PSP(lr);
#endif /* defined(ARCH_MPU_ARMv8M) */

    halt_info->valid_data = 0;
    if (no_halt_info) {
        return false;
    }

    /* CPU registers useful for debug. */
    halt_info->lr = lr;
    halt_info->ipsr = __get_IPSR();
    halt_info->control = __get_CONTROL();

    /* Save the status registers in the halt info. */
    halt_info->mmfar = SCB->MMFAR;
    halt_info->bfar = SCB->BFAR;
    halt_info->cfsr = SCB->CFSR;
    halt_info->hfsr = SCB->HFSR;
    halt_info->dfsr = SCB->DFSR;
    halt_info->afsr = SCB->AFSR;
    halt_info->valid_data |= HALT_INFO_REGISTERS;

    /* Copy the exception stack frame into the halt info.
     * Make sure that we're not dealing with stacking error in which case we
     * don't have a valid exception stack frame.
     * Stacking error may happen in case of Bus Fault, MemManage Fault or
     * their escalation to Hard Fault. */
    static const uint32_t stack_error_msk = SCB_CFSR_MSTKERR_Msk | SCB_CFSR_STKERR_Msk;
    if (!(halt_info->cfsr & stack_error_msk)) {
        memcpy((void *)&halt_info->stack_frame, (void *)sp,
            CONTEXT_SWITCH_EXC_SF_BYTES);
        halt_info->valid_data |= HALT_INFO_STACK_FRAME;
    }

    return true;
}
