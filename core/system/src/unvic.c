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
#include "halt.h"
#include "svc.h"
#include "unvic.h"

/* unprivileged vector table */
TIsrUVector g_unvic_vector[NVIC_VECTORS];
uint8_t g_nvic_prio_bits;

/* Counter to keep track of how many times a disable-all function has been
 * called for each box.
 *
 * @internal
 *
 * If multiple disable-/re-enable-all functions are called in the same box, a
 * counter is respectively incremented/drecremented so that it never happens
 * that a nested function re-enables IRQs for the caller. */
uint32_t g_irq_disable_all_counter[UVISOR_MAX_BOXES];

/* vmpu_acl_irq to unvic_acl_add */
void vmpu_acl_irq(uint8_t box_id, void *function, uint32_t irqn)
     UVISOR_LINKTO(unvic_acl_add);

static void unvic_default_check(uint32_t irqn)
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

void unvic_acl_add(uint8_t box_id, void *function, uint32_t irqn)
{
    TIsrUVector *uv;

    /* don't allow to modify uVisor-owned IRQs */
    unvic_default_check(irqn);

    /* get vector entry */
    uv = &g_unvic_vector[irqn];

    /* check if IRQ entry is populated */
    if(uv->id || uv->hdlr)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "Permission denied: IRQ %d is owned by box %d\n\r", irqn,
                                                                       uv->id);
    }

    /* save settings, function handler is optional */
    uv->id = box_id;
    uv->hdlr = function;
}

static int unvic_acl_check(int irqn)
{
    int is_irqn_registered;
    TIsrUVector *uv;

    /* don't allow to modify uVisor-owned IRQs */
    unvic_default_check(irqn);

    /* get vector entry */
    uv = &g_unvic_vector[irqn];

    /* an IRQn slot is considered as registered if the ISR entry in the unprivileged
     * table is not 0 */
    is_irqn_registered = uv->hdlr ? 1 : 0;

    /* check if the same box that registered the IRQn is accessing it
     * note: if the vector entry for a certain IRQn is 0, it means that no secure
     *       box claimed exclusive ownership for it. So, another box can claim it
     *       if it is currently un-registered (that is, if the registered handler
     *       is NULL) */
    if(uv->id != g_active_box && (uv->id || is_irqn_registered))
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "Permission denied: IRQ %d is owned by box %d\n\r", irqn,
                                                                       uv->id);
    }

    return is_irqn_registered;
}

void unvic_isr_set(uint32_t irqn, uint32_t vector)
{
    /* verify IRQ access privileges */
    unvic_acl_check(irqn);

    /* save unprivileged handler
     * note: if the handler is NULL the IRQn gets de-registered for the current
     *       box, meaning that other boxes can still use it. In this way boxes
     *       can share un-exclusive IRQs. A secure box should not do that if it
     *       wants to hold exclusivity over an IRQn
     * note: when an IRQ is released (de-registered) the corresponding IRQn is
     *       disabled, to ensure that no spurious interrupts are served */
    if (vector) {
        g_unvic_vector[irqn].id = g_active_box;
    }
    else {
        NVIC_DisableIRQ(irqn);
        g_unvic_vector[irqn].id = 0;
    }
    g_unvic_vector[irqn].hdlr = (TIsrVector) vector;

    /* set default priority (SVC must always be higher) */
    NVIC_SetPriority(irqn, __UVISOR_NVIC_MIN_PRIORITY);

    DPRINTF("IRQ %d %s box %d\n\r",
            irqn,
            vector ? "registered to" : "released by",
            g_active_box);
}

uint32_t unvic_isr_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_acl_check(irqn);

    return (uint32_t) g_unvic_vector[irqn].hdlr;
}

void unvic_irq_enable(uint32_t irqn)
{
    int is_irqn_registered;

    /* verify IRQ access privileges */
    is_irqn_registered = unvic_acl_check(irqn);

    /* Enable the IRQ, but only if IRQs are not globally disabled for the
     * currently active box. */
    if (is_irqn_registered) {
        /* If the counter of nested disable-all IRQs is set to 0, it means that
         * IRQs are not globally disabled for the current box. */
        if (!g_irq_disable_all_counter[g_active_box]) {
            DPRINTF("IRQ %d enabled\n\r", irqn);
            NVIC_EnableIRQ(irqn);
        } else {
            /* We do not enable the IRQ directly, but notify uVisor to enable it
             * when IRQs will be re-enabled globally for the current box. */
            g_unvic_vector[irqn].was_enabled = true;
        }
        return;
    }
    else if (UNVIC_IS_IRQ_ENABLED(irqn)) {
        DPRINTF("IRQ %d is unregistered; state unchanged\n\r", irqn);
        return;
    }
    else {
        HALT_ERROR(NOT_ALLOWED, "IRQ %d is unregistered; state cannot be changed", irqn);
    }
}

void unvic_irq_disable(uint32_t irqn)
{
    int is_irqn_registered;

    /* verify IRQ access privileges */
    is_irqn_registered = unvic_acl_check(irqn);

    if (is_irqn_registered) {
        DPRINTF("IRQ %d disabled, but still owned by box %d\n\r", irqn, g_unvic_vector[irqn].id);
        NVIC_DisableIRQ(irqn);
        return;
    }
    else if (!UNVIC_IS_IRQ_ENABLED(irqn)) {
        DPRINTF("IRQ %d is unregistered; state unchanged\n\r", irqn);
        return;
    }
    else {
        HALT_ERROR(NOT_ALLOWED, "IRQ %d is unregistered; state cannot be changed", irqn);
    }
}

/** Disable all interrupts for the currently active box.
 *
 * @internal
 *
 * This function keeps a state in the unprivileged vector table that will be
 * used later on, in ::unvic_irq_enable_all, to re-enabled previously disabled
 * IRQs. */
void unvic_irq_disable_all(void)
{
    int irqn;

    /* Only disable all IRQs if this is the first time that this function is
     * called. */
    if (g_irq_disable_all_counter[g_active_box] == 0) {
        /* Iterate over all the IRQs owned by the currently active box and
         * disable them if they were active before the function call. */
        for (irqn = 0; irqn < NVIC_VECTORS; irqn++) {
            if (g_unvic_vector[irqn].id == g_active_box && UNVIC_IS_IRQ_ENABLED(irqn)) {
                /* Remember the state for this IRQ. The state is the NVIC one,
                 * so we are sure we don't enable spurious interrupts. */
                g_unvic_vector[irqn].was_enabled = true;

                /* Disable the IRQ. */
                NVIC_DisableIRQ(irqn);
            } else {
                g_unvic_vector[irqn].was_enabled = false;
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
 * The state that was previously set in ::unvic_irq_enable_all is reset, so that
 * spurious calls to this function do not mistakenly enable IRQs that are
 * supposed to be disabled.
 *
 * IRQs are only re-enabled if the internal counter set by
 * ::unvic_irq_disable_all reaches 0. */
void unvic_irq_enable_all(void)
{
    int irqn;

    /* Only re-enable all IRQs if this is the last time that this function is
     * called. */
    if (g_irq_disable_all_counter[g_active_box] == 1) {
        /* Iterate over all the IRQs owned by the currently active box and
         * re-enable them if they were either (i.) enabled before the
         * disable-all phase, or (ii.) enabled during the disable-all phase. */
        for (irqn = 0; irqn < NVIC_VECTORS; irqn++) {
            if (g_unvic_vector[irqn].id == g_active_box && g_unvic_vector[irqn].was_enabled) {
                /* Re-enable the IRQ. */
                NVIC_EnableIRQ(irqn);

                /* Reset the state. This is only needed in case someone calls
                 * this function without having previously called the
                 * disable-all one. */
                g_unvic_vector[irqn].was_enabled = false;
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

void unvic_irq_pending_clr(uint32_t irqn)
{
    int is_irqn_registered;

    /* verify IRQ access privileges */
    is_irqn_registered = unvic_acl_check(irqn);

    /* enable IRQ */
    if (is_irqn_registered) {
        DPRINTF("IRQ %d pending status cleared\n\r", irqn);
        NVIC_ClearPendingIRQ(irqn);
        return;
    }
    else {
        HALT_ERROR(NOT_ALLOWED, "IRQ %d is unregistered; state cannot be changed", irqn);
    }
}

void unvic_irq_pending_set(uint32_t irqn)
{
    int is_irqn_registered;

    /* verify IRQ access privileges */
    is_irqn_registered = unvic_acl_check(irqn);

    /* enable IRQ */
    if (is_irqn_registered) {
        DPRINTF("IRQ %d pending status set (will be served as soon as possible)\n\r", irqn);
        NVIC_SetPendingIRQ(irqn);
        return;
    }
    else {
        HALT_ERROR(NOT_ALLOWED, "IRQ %d is unregistered; state cannot be changed", irqn);
    }
}

uint32_t unvic_irq_pending_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_acl_check(irqn);

    /* get priority for device specific interrupts  */
    return NVIC_GetPendingIRQ(irqn);
}

void unvic_irq_priority_set(uint32_t irqn, uint32_t priority)
{
    int is_irqn_registered;

    /* verify IRQ access privileges */
    is_irqn_registered = unvic_acl_check(irqn);

    /* check for maximum priority */
    if (priority > UVISOR_VIRQ_MAX_PRIORITY) {
        HALT_ERROR(NOT_ALLOWED, "NVIC priority overflow; max priority allowed: %d\n\r", UVISOR_VIRQ_MAX_PRIORITY);
    }

    /* set priority for device specific interrupts */
    if (is_irqn_registered) {
        DPRINTF("IRQ %d priority set to %d (NVIC), %d (virtual)\n\r", irqn, __UVISOR_NVIC_MIN_PRIORITY + priority,
                                                                            priority);
        NVIC_SetPriority(irqn, __UVISOR_NVIC_MIN_PRIORITY + priority);
        return;
    }
    else {
        HALT_ERROR(NOT_ALLOWED, "IRQ %d is unregistered; state cannot be changed", irqn);
    }
}

uint32_t unvic_irq_priority_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_acl_check(irqn);

    /* get priority for device specific interrupts  */
    return NVIC_GetPriority(irqn) - __UVISOR_NVIC_MIN_PRIORITY;
}

int unvic_irq_level_get(void)
{
    /* gather IPSR from exception stack frame */
    /* the currently active IRQn is the one of the SVCall, while instead we want
     * to know the IRQn at the time of the SVCcall, which is saved on the stack */
    uint32_t ipsr = vmpu_unpriv_uint32_read(__get_PSP() + 4 * 7);
    uint32_t irqn = (ipsr & 0x1FF) - NVIC_OFFSET;

    /* check if an interrupt is actually active */
    /* note: this includes pending interrupts that are not currently being served */
    if (!ipsr || !NVIC_GetActive(irqn)) {
        return -1;
    }

    /* check that the IRQn is not owned by uVisor */
    /* this also checks that the IRQn is in the correct range */
    unvic_default_check(irqn);

    /* if an IRQn is active, return the (virtualised, i.e. shifted) priority level
     * of the interrupt, which goes from 0 up */
    return (int) NVIC_GetPriority(irqn) - __UVISOR_NVIC_MIN_PRIORITY;
}

/* naked wrapper function needed to preserve LR */
void UVISOR_NAKED unvic_svc_cx_in(uint32_t *svc_sp, uint32_t svc_pc)
{
    asm volatile(
        "push {r4 - r11}\n"         /* store callee-saved regs on MSP (priv) */
        "push {lr}\n"               /* save lr for later */
        // r0 = svc_sp              /* 1st arg: SVC sp */
        // r1 = svc_pc              /* 1st arg: SVC pc */
        "bl   __unvic_svc_cx_in\n"  /* execute handler and return */
        "cmp  r0, #0\n"             /* check return flag */
        "beq  no_regs_clearing\n"
        "mov  r4,  #0\n"            /* clear callee-saved regs on request */
        "mov  r5,  #0\n"
        "mov  r6,  #0\n"
        "mov  r7,  #0\n"
        "mov  r8,  #0\n"
        "mov  r9,  #0\n"
        "mov  r10, #0\n"
        "mov  r11, #0\n"
        "no_regs_clearing:\n"
        "pop  {lr}\n"               /* restore lr */
        "orr  lr, #0x1C\n"          /* return with PSP, 8 words */
        "bx   lr\n"
        /* the unprivileged interrupt handler will be executed after this */
        :: "r" (svc_sp), "r" (svc_pc)
    );
}

uint32_t __unvic_svc_cx_in(uint32_t *svc_sp, uint32_t svc_pc)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_fn;
    uint32_t           dst_sp_align;

    /* this handler is always executed from privileged code */
    uint32_t *msp = svc_sp;

    /* gather information from IRQn */
    uint32_t ipsr = msp[7];
    uint32_t irqn = (ipsr & 0x1FF) - NVIC_OFFSET;

    dst_id = g_unvic_vector[irqn].id;
    dst_fn = (uint32_t) g_unvic_vector[irqn].hdlr;

    /* verify IRQ access privileges */
    /* note: only default basic checks are performed, since the remaining
     * information is fetched from our trusted vector table */
    unvic_default_check(irqn);

    /* check ISR vector */
    if(!dst_fn)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "Unprivileged handler for IRQ %i not found\n\r", irqn);
    }

    /* gather information from current state */
    src_id = g_active_box;
    src_sp = svc_cx_validate_sf((uint32_t *) __get_PSP());

    /* a proper context switch is only needed if changing box */
    if(src_id != dst_id)
    {
        /* gather information from current state */
        dst_sp = svc_cx_get_curr_sp(dst_id);

        /* set the context stack pointer for the dst box */
        *(__uvisor_config.uvisor_box_context) = g_svc_cx_context_ptr[dst_id];

        /* switch boxes */
        vmpu_switch(src_id, dst_id);
    }
    else
    {
        dst_sp = src_sp;
    }

    /* create unprivileged stack frame */
    dst_sp_align = ((uint32_t) dst_sp & 0x4) ? 1 : 0;
    dst_sp      -= (SVC_CX_EXC_SF_SIZE - dst_sp_align);

    /* create stack frame for de-privileging the interrupt:
     *   - all general purpose registers are ignored
     *   - lr is the next SVCall
     *   - the return address is the unprivileged handler
     *   - xPSR is copied from the MSP
     *   - pending IRQs are cleared in iPSR
     *   - store alignment for future unstacking */
    dst_sp[5] = ((uint32_t) svc_pc + 2) | 1;             /* lr */
    dst_sp[6] = dst_fn;                                  /* return address */
    dst_sp[7] = ipsr;                                    /* xPSR */
    dst_sp[7] &= ~0x1FF;                                 /* IPSR - clear IRQn */
    dst_sp[7] |= dst_sp_align << 9;                      /* xPSR - alignment */

    /* save the current state */
    svc_cx_push_state(src_id, TBOXCX_IRQ, src_sp, dst_id);
    /* DEBUG_CX_SWITCH_IN(); */

    /* de-privilege executionn */
    __set_PSP((uint32_t) dst_sp);
    __set_CONTROL(__get_CONTROL() | 3);

    /* DPRINTF("IRQ %d served with vector 0x%08X\n\r", irqn, */
    /*                                                 g_unvic_vector[irqn]); */
    /* FIXME add support for privacy (triggers register clearing) */
    return 0;
}

/* naked wrapper function needed to preserve MSP and LR */
void UVISOR_NAKED unvic_svc_cx_out(uint32_t *svc_sp)
{
    asm volatile(
        // r0 = svc_sp                  /* 1st arg: SVC sp */
        "mrs r1, MSP\n"                 /* 2nd arg: msp */
        "add r1, #32\n"                 /* account for callee-saved registers */
        "push {lr}\n"                   /* save lr for later */
        "bl __unvic_svc_cx_out\n"       /* execute handler and return */
        "pop  {lr}\n"                   /* restore lr */
        "pop  {r4-r11}\n"               /* restore callee-saved registers */
        "orr lr, #0x10\n"               /* return with MSP, 8 words */
        "bic lr, #0xC\n"
        "bx  lr\n"
        :: "r" (svc_sp)
    );
}

void __unvic_svc_cx_out(uint32_t *svc_sp, uint32_t *msp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

    /* gather information from current state */
    dst_id = g_active_box;
    dst_sp = svc_cx_validate_sf(svc_sp);

    /* gather information from previous state */
    svc_cx_pop_state(dst_id, dst_sp);
    src_id = svc_cx_get_src_id();
    src_sp = svc_cx_get_src_sp();
    /* DEBUG_CX_SWITCH_OUT(); */

    /* copy return address of previous stack frame to the privileged one, which
     * was kept idle after interrupt de-privileging */
    msp[6] = dst_sp[6];

    if(src_id != dst_id)
    {
        /* set the context stack pointer back to the one of the src box */
        *(__uvisor_config.uvisor_box_context) = g_svc_cx_context_ptr[src_id];

        /* switch ACls */
        vmpu_switch(dst_id, src_id);
    }

    /* re-privilege execution */
    __set_PSP((uint32_t) src_sp);
    __set_CONTROL(__get_CONTROL() & ~2);
}

void unvic_init(void)
{
    uint8_t prio_bits;
    uint8_t volatile *prio;

    /* Detect the number of implemented priority bits.
     * The architecture specifies that unused/not implemented bits in the
     * NVIC IP registers read back as 0. */
    __disable_irq();
    prio = (uint8_t volatile *) &(NVIC->IP[0]);
    prio_bits = *prio;
    *prio = 0xFFU;
    g_nvic_prio_bits = (uint8_t) __builtin_popcount(*prio);
    *prio = prio_bits;
    __enable_irq();

    /* Verify that the priority bits read at runtime are realistic. */
    assert(g_nvic_prio_bits > 0 && g_nvic_prio_bits <= 8);

    /* check that minimum priority is still in the range of possible priority
     * levels */
    assert(__UVISOR_NVIC_MIN_PRIORITY < UVISOR_VIRQ_MAX_PRIORITY);

    /* by setting the priority group to 0 we make sure that all priority levels
     * are available for pre-emption and that interrupts with the same priority
     * level occurring at the same time are served in the default way, that is,
     * by IRQ number
     * for example, IRQ 0 has precedence over IRQ 1 if both have the same
     * priority level */
    NVIC_SetPriorityGrouping(0);
}
