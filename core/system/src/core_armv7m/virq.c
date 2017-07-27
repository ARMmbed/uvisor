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
#include "debug.h"
#include "context.h"
#include "halt.h"
#include "svc.h"
#include "virq.h"
#include "vmpu.h"

/* Define the correct NVIC Priority Register name. */
#if defined(CORE_CORTEX_M33)
#define NVIC_IPR NVIC->IPR
#else /* defined(CORE_CORTEX_M33) */
#define NVIC_IPR NVIC->IP
#endif /* defined(CORE_CORTEX_M33) */

#define VIRQ_ISR_OWNER_OTHER 0
#define VIRQ_ISR_OWNER_NONE  1
#define VIRQ_ISR_OWNER_SELF  2

/* unprivileged vector table */
TIsrUVector g_virq_vector[NVIC_VECTORS];
uint8_t g_virq_prio_bits;

/* Counter to keep track of how many times a disable-all function has been
 * called for each box.
 *
 * @internal
 *
 * If multiple disable-/re-enable-all functions are called in the same box, a
 * counter is respectively incremented/decremented so that it never happens
 * that a nested function re-enables IRQs for the caller. */
uint32_t g_irq_disable_all_counter[UVISOR_MAX_BOXES];

static void virq_default_check(uint32_t irqn)
{
    /* IRQn goes from 0 to (NVIC_VECTORS - 1) */
    if(irqn >= NVIC_VECTORS)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "Not allowed: IRQ %d is out of range\n\r", irqn);
    }

    /* check if uvisor does not already own the IRQn slot */
    if(g_isr_vector[NVIC_OFFSET + irqn] != &isr_default_handler)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "Permission denied: IRQ %d is owned by uVisor\n\r", irqn);
    }
}

static int virq_acl_check(int irqn)
{
    TIsrUVector *uv;

    /* don't allow to modify uVisor-owned IRQs */
    virq_default_check(irqn);

    /* get vector entry */
    uv = &g_virq_vector[irqn];

    if (uv->id == UVISOR_BOX_ID_INVALID) {
        return VIRQ_ISR_OWNER_NONE;
    }
    if (uv->id == g_active_box) {
        return VIRQ_ISR_OWNER_SELF;
    }
    return VIRQ_ISR_OWNER_OTHER;
}

static void virq_isr_register(uint32_t irqn)
{
    switch (virq_acl_check(irqn))
    {
        case VIRQ_ISR_OWNER_NONE:
            g_virq_vector[irqn].id = g_active_box;
            DPRINTF("IRQ %d registered to box %d\n\r", irqn, g_active_box);
        case VIRQ_ISR_OWNER_SELF:
            return;
        default:
            break;
    }
    HALT_ERROR(PERMISSION_DENIED, "Permission denied: IRQ %d is owned by another box!\r\n", irqn);
}

void virq_acl_add(uint8_t box_id, uint32_t irqn)
{
    /* Only save the IRQ if it's not owned by anybody else. */
    int owner = virq_acl_check(irqn);
    if (owner == VIRQ_ISR_OWNER_OTHER) {
        HALT_ERROR(PERMISSION_DENIED, "vIRQ: IRQ %d is already owned by box %u.\r\n", irqn, g_virq_vector[irqn].id);
    }
    g_virq_vector[irqn].id = box_id;
}

void virq_isr_set(uint32_t irqn, uint32_t vector)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Save unprivileged handler. */
    g_virq_vector[irqn].hdlr = (TIsrVector) vector;
}

uint32_t virq_isr_get(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    return (uint32_t) g_virq_vector[irqn].hdlr;
}

void virq_irq_enable(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* If the counter of nested disable-all IRQs is set to 0, it means that
     * IRQs are not globally disabled for the current box. */
    if (!g_irq_disable_all_counter[g_active_box]) {
        DPRINTF("IRQ %d enabled\n\r", irqn);
        NVIC_EnableIRQ(irqn);
    } else {
        /* We do not enable the IRQ directly, but notify uVisor to enable it
         * when IRQs will be re-enabled globally for the current box. */
        g_virq_vector[irqn].was_enabled = true;
    }
    return;
}

void virq_irq_disable(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    DPRINTF("IRQ %d disabled, but still owned by box %d\n\r", irqn, g_virq_vector[irqn].id);
    NVIC_DisableIRQ(irqn);
    return;
}

/** Disable all interrupts for the currently active box.
 *
 * @internal
 *
 * This function keeps a state in the unprivileged vector table that will be
 * used later on, in ::virq_irq_enable_all, to re-enabled previously disabled
 * IRQs. */
void virq_irq_disable_all(void)
{
    int irqn;

    /* Only disable all IRQs if this is the first time that this function is
     * called. */
    if (g_irq_disable_all_counter[g_active_box] == 0) {
        /* Iterate over all the IRQs owned by the currently active box and
         * disable them if they were active before the function call. */
        for (irqn = 0; irqn < NVIC_VECTORS; irqn++) {
            if (g_virq_vector[irqn].id == g_active_box && VIRQ_IS_IRQ_ENABLED(irqn)) {
                /* Remember the state for this IRQ. The state is the NVIC one,
                 * so we are sure we don't enable spurious interrupts. */
                g_virq_vector[irqn].was_enabled = true;

                /* Disable the IRQ. */
                NVIC_DisableIRQ(irqn);
            } else {
                g_virq_vector[irqn].was_enabled = false;
            }
        }
    }

    /* Increment the counter of disable-all calls. */
    if (g_irq_disable_all_counter[g_active_box] < UINT32_MAX - 1) {
        g_irq_disable_all_counter[g_active_box]++;
    } else {
        HALT_ERROR(SANITY_CHECK_FAILED, "Overflow of the disable-all calls counter.");
    }

    if (g_irq_disable_all_counter[g_active_box] == 1) {
        DPRINTF("All IRQs for box %d have been disabled.\r\n", g_active_box);
    } else {
        DPRINTF("IRQs still disabled for box %d. Counter: %d.\r\n",
                g_active_box, g_irq_disable_all_counter[g_active_box]);
    }
}

/** Re-enable all previously interrupts for the currently active box.
 *
 * @internal
 *
 * The state that was previously set in ::virq_irq_enable_all is reset, so that
 * spurious calls to this function do not mistakenly enable IRQs that are
 * supposed to be disabled.
 *
 * IRQs are only re-enabled if the internal counter set by
 * ::virq_irq_disable_all reaches 0. */
void virq_irq_enable_all(void)
{
    int irqn;

    /* Only re-enable all IRQs if this is the last time that this function is
     * called. */
    if (g_irq_disable_all_counter[g_active_box] == 1) {
        /* Iterate over all the IRQs owned by the currently active box and
         * re-enable them if they were either (i.) enabled before the
         * disable-all phase, or (ii.) enabled during the disable-all phase. */
        for (irqn = 0; irqn < NVIC_VECTORS; irqn++) {
            if (g_virq_vector[irqn].id == g_active_box && g_virq_vector[irqn].was_enabled) {
                /* Re-enable the IRQ. */
                NVIC_EnableIRQ(irqn);

                /* Reset the state. This is only needed in case someone calls
                 * this function without having previously called the
                 * disable-all one. */
                g_virq_vector[irqn].was_enabled = false;
            }
        }
    }

    /* Decrease the counter of calls to disable-all. */
    /* Note: We do not fail explicitly if the counter is 0, because it just
     * means someone enabled all IRQs without having disabled them first, which
     * is acceptable (in one single box). */
    if (g_irq_disable_all_counter[g_active_box] > 0) {
        g_irq_disable_all_counter[g_active_box]--;
    }

    if (!g_irq_disable_all_counter[g_active_box]) {
        DPRINTF("All IRQs for box %d have been re-enabled.\r\n", g_active_box);
    } else {
        DPRINTF("IRQs still disabled for box %d. Counter: %d.\r\n",
                g_active_box, g_irq_disable_all_counter[g_active_box]);
    }
}

void virq_irq_pending_clr(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Clear pending IRQ. */
    DPRINTF("IRQ %d pending status cleared\n\r", irqn);
    NVIC_ClearPendingIRQ(irqn);
}

void virq_irq_pending_set(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Set pending IRQ. */
    DPRINTF("IRQ %d pending status set (will be served as soon as possible)\n\r", irqn);
    NVIC_SetPendingIRQ(irqn);
}

uint32_t virq_irq_pending_get(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Get priority for device specific interrupts. */
    return NVIC_GetPendingIRQ(irqn);
}

void virq_irq_priority_set(uint32_t irqn, uint32_t priority)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Check for maximum priority. */
    if (priority > UVISOR_VIRQ_MAX_PRIORITY) {
        HALT_ERROR(NOT_ALLOWED, "NVIC priority overflow; max priority allowed: %d\n\r", UVISOR_VIRQ_MAX_PRIORITY);
    }

    /* Set priority for device specific interrupts. */
    DPRINTF("IRQ %d priority set to %d (NVIC), %d (virtual)\n\r", irqn, __UVISOR_NVIC_MIN_PRIORITY + priority,
                                                                        priority);
    NVIC_SetPriority(irqn, __UVISOR_NVIC_MIN_PRIORITY + priority);
}

uint32_t virq_irq_priority_get(uint32_t irqn)
{
    /* This function halts if the IRQ is owned by another box or by uVisor. */
    virq_isr_register(irqn);

    /* Get priority for device specific interrupts. */
    return NVIC_GetPriority(irqn) - __UVISOR_NVIC_MIN_PRIORITY;
}

int virq_irq_level_get(void)
{
    /* Gather IPSR from exception stack frame. */
    /* The currently active IRQn is the one of the SVCall, while instead we want
     * to know the IRQn at the time of the SVCcall, which is saved on the stack. */
    uint32_t ipsr = vmpu_unpriv_uint32_read(__get_PSP() + 4 * 7);
    uint32_t irqn = (ipsr & 0x1FF) - NVIC_OFFSET;

    /* Check if an interrupt is actually active. */
    /* Note: This includes pending interrupts that are not currently being served. */
    if (!ipsr || !NVIC_GetActive(irqn)) {
        return -1;
    }

    /* Check that the IRQn is not owned by uVisor. */
    /* This also checks that the IRQn is in the correct range. */
    virq_default_check(irqn);

    /* If an IRQn is active, return the (virtualised, i.e. shifted) priority level
     * of the interrupt, which goes from 0 up. */
    return (int) NVIC_GetPriority(irqn) - __UVISOR_NVIC_MIN_PRIORITY;
}

/** Perform a context switch-in as a result of an interrupt request.
 *
 * @internal
 *
 * This function is implemented as a wrapper, needed to make sure that the lr
 * register doesn't get polluted and to provide context privacy during a context
 * switch. The actual function is ::virq_gateway_context_switch_in. The wrapper
 * also changes the lr register so that we can return to a different privilege
 * level. */
void UVISOR_NAKED virq_gateway_in(uint32_t svc_sp, uint32_t svc_pc)
{
    /* According to the ARM ABI, r0 and r1 will have the following values when
     * this function is called:
     *   r0 = svc_sp
     *   r1 = svc_pc */
    asm volatile(
        "push {r4 - r11}\n"                             /* Store the callee-saved registers on the MSP (privileged). */
        "push {lr}\n"                                   /* Preserve the lr register. */
        "bl   virq_gateway_context_switch_in\n"         /* privacy = virq_context_switch_in(svc_sp, svc_pc) */
        "cmp  r0, #0\n"                                 /* if (privacy)  */
        "beq  virq_gateway_no_regs_clearing\n"          /* {             */
        "mov  r4,  #0\n"                                /*     Clear r4  */
        "mov  r5,  #0\n"                                /*     Clear r5  */
        "mov  r6,  #0\n"                                /*     Clear r6  */
        "mov  r7,  #0\n"                                /*     Clear r7  */
        "mov  r8,  #0\n"                                /*     Clear r8  */
        "mov  r9,  #0\n"                                /*     Clear r9  */
        "mov  r10, #0\n"                                /*     Clear r10 */
        "mov  r11, #0\n"                                /*     Clear r11 */
        "virq_gateway_no_regs_clearing:\n"              /* } else { ; }  */
        "pop  {lr}\n"                                   /* Restore the lr register. */
        "orr  lr, #0x1C\n"                              /* Return to unprivileged mode, using the PSP, 8 words stack. */
        "bx   lr\n"                                     /* Return. Note: Callee-saved registers are not popped here. */
                                                        /* The destination ISR will be executed after this. */
        :: "r" (svc_sp), "r" (svc_pc)
    );
}

/** Perform a context switch-in as a result of an interrupt request.
 *
 * @internal
 *
 * This function implements ::virq_gateway_in, which is instead only a wrapper.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the interrupt
 * @param svc_pc[in]    Program counter at the time of the interrupt */
uint32_t virq_gateway_context_switch_in(uint32_t svc_sp, uint32_t svc_pc)
{
    uint8_t dst_id;
    uint32_t dst_fn;
    uint32_t src_sp, dst_sp, msp;
    uint32_t ipsr, irqn, xpsr;
    uint32_t virq_thunk;

    /* This handler is always executed from privileged code, so the SVCall stack
     * pointer is the MSP. */
    msp = svc_sp;

    /* Destination box: Gather information from the IRQn. */
    ipsr = ((uint32_t *) msp)[7];
    irqn = (ipsr & 0x1FF) - NVIC_OFFSET;
    dst_id = g_virq_vector[irqn].id;
    dst_fn = (uint32_t) g_virq_vector[irqn].hdlr;

    /* Verify the IRQn access privileges. */
    /* Note: Only default basic checks are performed, since the remaining
     * information is fetched from our trusted vector table. */
    virq_default_check(irqn);

    /* Check if the ISR is registered. */
    if(!dst_fn) {
        HALT_ERROR(NOT_ALLOWED, "Unprivileged handler for IRQ %i not found\n\r", irqn);
    }

    /* Source box: Get the current stack pointer. */
    /* Note: We must use the current PSP as the SVCall can only give us the MSP
     * register, since it was triggered from privileged mode. */
    src_sp = context_validate_exc_sf(__get_PSP());

    /* The stacked xPSR register clears the IRQn. */
    xpsr = ipsr & ~0x1FF;

    /* The stacked return value is the second SVCall in the uVisor default ISR
     * handler. */
    virq_thunk = (uint32_t) (svc_pc + 2);

    /* Forge a stack frame for the destination box. */
    dst_sp = context_forge_exc_sf(src_sp, dst_id, dst_fn, virq_thunk, xpsr, 0);

    /* Perform the context switch-in to the destination box. */
    /* This function halts if it finds an error. */
    context_switch_in(CONTEXT_SWITCH_FUNCTION_ISR, dst_id, src_sp, dst_sp);

    /* De-privilege execution. */
    __set_CONTROL(__get_CONTROL() | 3);

    /* ISB to ensure subsequent instructions are fetched with the correct privilege level */
    __ISB();

    /* Return whether the destination box requires privacy or not. */
    /* TODO: Context privacy is currently unsupported. */
    return 0;
}

/** Perform a context switch-out as a result of an interrupt request.
 *
 * @internal
 *
 * This function is implemented as a wrapper, needed to make sure that the lr
 * register doesn't get polluted and to provide context privacy during a context
 * switch. The actual function is ::virq_gateway_context_switch_out. The
 * wrapper also changes the lr register so that we can return to a different
 * privilege level. */
void UVISOR_NAKED virq_gateway_out(uint32_t svc_sp)
{
    /* According to the ARM ABI, r0 will have the following value when this
     * function is called:
     *   r0 = svc_sp
     * In addition, we will be passing also r1 to the target function:
     *   r1 = MSP */
    asm volatile(
        "mrs r1, MSP\n"                             /* Read the MSP. */
        "add r1, #32\n"                             /* Account for the previously pushed callee-saved registers. */
        "push {lr}\n"                               /* Save the lr register for later. */
        "bl virq_gateway_context_switch_out\n"      /* virq_gateway_context_switch_out(svc_sp, msp) */
        "pop  {lr}\n"                               /* Restore the lr register. */
        "pop  {r4-r11}\n"                           /* Restore the previously saved callee-saved registers. */
        "orr lr, #0x10\n"                           /* Return to unprivileged mode, using the MSP, 8 words stack */
        "bic lr, #0xC\n"
        "bx  lr\n"                                  /* Return. */
        :: "r" (svc_sp)
    );
}

/** Perform a context switch-out from a previous interrupt request.
 *
 * @internal
 *
 * This function implements ::virq_gateway_out, which is instead only a
 * wrapper.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the interrupt
 *                      return handler (thunk)
 * @param msp[in]       Value of the MSP register at the time of the SVcall */
void virq_gateway_context_switch_out(uint32_t svc_sp, uint32_t msp)
{
    uint32_t dst_sp;

    /* Copy the return address of the previous stack frame to the privileged
     * one, which was kept idle after interrupt de-privileging */
    dst_sp = context_validate_exc_sf(svc_sp);
    ((uint32_t *) msp)[6] = ((uint32_t *) dst_sp)[6];

    /* Discard the unneeded exception stack frame from the destination box
     * stack. The destination box is the currently active one. */
    context_discard_exc_sf(g_active_box, dst_sp);

    /* Perform the context switch back to the previous state. */
    context_switch_out(CONTEXT_SWITCH_FUNCTION_ISR);

    /* Re-privilege execution. */
    __set_CONTROL(__get_CONTROL() & ~2);

    /* ISB to ensure subsequent instructions are fetched with the correct privilege level */
    __ISB();
}

void virq_init(uint32_t const * const user_vtor)
{
    uint8_t prio_bits;
    uint8_t volatile *prio;

    /* Detect the number of implemented priority bits.
     * The architecture specifies that unused/not implemented bits in the
     * NVIC IP registers read back as 0. */
    __disable_irq();
    prio = (uint8_t volatile *) &(NVIC_IPR[0]);
    prio_bits = *prio;
    *prio = 0xFFU;
    g_virq_prio_bits = (uint8_t) __builtin_popcount(*prio);
    *prio = prio_bits;
    __enable_irq();

    /* Verify that the priority bits read at runtime are realistic. */
    assert(g_virq_prio_bits > 0 && g_virq_prio_bits <= 8);

    /* Check that minimum priority is still in the range of possible priority
     * levels. */
    assert(__UVISOR_NVIC_MIN_PRIORITY < UVISOR_VIRQ_MAX_PRIORITY);

    /* Set the priority of each exception. SVC is lower priority than
     * MemManage, BusFault, UsageFault, and SecureFault_IRQn, so that we can
     * recover from security violations more simply. */
    static const uint32_t priority_0 = __UVISOR_NVIC_MIN_PRIORITY - 2;
    static const uint32_t priority_1 = __UVISOR_NVIC_MIN_PRIORITY - 1;
    assert(priority_0 < __UVISOR_NVIC_MIN_PRIORITY);
    assert(priority_1 < __UVISOR_NVIC_MIN_PRIORITY);
    NVIC_SetPriority(MemoryManagement_IRQn, priority_0);
    NVIC_SetPriority(BusFault_IRQn, priority_0);
    NVIC_SetPriority(UsageFault_IRQn, priority_0);
    NVIC_SetPriority(SVCall_IRQn, priority_1);
    NVIC_SetPriority(DebugMonitor_IRQn, __UVISOR_NVIC_MIN_PRIORITY);
    NVIC_SetPriority(PendSV_IRQn, UVISOR_VIRQ_MAX_PRIORITY);
    NVIC_SetPriority(SysTick_IRQn, UVISOR_VIRQ_MAX_PRIORITY);

    /* By setting the priority group to 0 we make sure that all priority levels
     * are available for pre-emption and that interrupts with the same priority
     * level occurring at the same time are served in the default way, that is,
     * by IRQ number. For example, IRQ 0 has precedence over IRQ 1 if both have
     * the same priority level. */
    NVIC_SetPriorityGrouping(0);

    /* Copy the user interrupt table into the handlers to prevent user code
     * from failing when enabling the IRQ before setting it.
     * Note that the IRQ may fire and use the statically declared IRQ handler
     * _before_ the user has had a chance to set a custom (runtime) handler.
     * This mirrors hardware behavior. */
    for (uint32_t ii = 0; ii < NVIC_VECTORS; ii++) {
        g_virq_vector[ii].id = UVISOR_BOX_ID_INVALID;
        g_virq_vector[ii].hdlr = (TIsrVector) user_vtor[ii + NVIC_OFFSET];
        /* Set default priority (SVC must always be higher). */
        NVIC_SetPriority(ii, __UVISOR_NVIC_MIN_PRIORITY);
    }
}
