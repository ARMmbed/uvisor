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
#include "halt.h"
#include "virq.h"
#include "vmpu.h"
#include <stdint.h>

/* Number of implemented and available priority bits. */
uint8_t g_virq_prio_bits;

/* IRQs state */
typedef struct virq_state_t {
    uint8_t box_id;
    bool enabled;
    uint8_t priority;
} TVirqState;
static TVirqState g_virq_states[NVIC_VECTORS];

/* SysTick state */
typedef struct virq_systick_state_t {
    SysTick_Type periph;
    bool active;
    uint8_t priority;
} TVirqSysTickState;
static TVirqSysTickState g_virq_state_systick_ns;

/* Per-box state that keeps track if the box was pre-empted while in an IRQ. */
static bool g_virq_box_in_active_irq[UVISOR_MAX_BOXES];

/* Minimum priority that a user can assign to an IRQ. */
/* Note: The minimum priority is actually the maximum priority value. */
static uint8_t g_virq_min_priority;

static void virq_check_acls(uint32_t irqn, uint8_t box_id)
{
    /* IRQn goes from 0 to (NVIC_VECTORS - 1) */
    if (irqn >= NVIC_VECTORS) {
        HALT_ERROR(NOT_ALLOWED, "Not allowed: IRQ %d is out of range\n\r", irqn);
    }

    /* Note: IRQs ownership is determined on a first come first served basis. */
    if (g_virq_states[irqn].box_id != UVISOR_BOX_ID_INVALID &&
        g_virq_states[irqn].box_id != box_id) {
        HALT_ERROR(PERMISSION_DENIED, "IRQ %d is owned by box %d.\r\n",
                   irqn, box_id);
    }
}

static inline void virq_disable_systick_ns(void)
{
    SysTick_NS->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
}

static inline void virq_enable_systick_ns(void)
{
    SysTick_NS->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

static inline bool virq_get_active_systick_ns(void)
{
    return (bool) (SCB_NS->SHCSR & SCB_SHCSR_SYSTICKACT_Msk);
}

static inline void virq_set_active_systick_ns(bool activate)
{
    if (activate) {
        SCB_NS->SHCSR |= SCB_SHCSR_SYSTICKACT_Msk;
    } else {
        SCB_NS->SHCSR &= ~SCB_SHCSR_SYSTICKACT_Msk;
    }
}

static void virq_copy_systick_ns(void)
{
    g_virq_state_systick_ns.periph.CTRL = SysTick_NS->CTRL;
    g_virq_state_systick_ns.periph.LOAD = SysTick_NS->LOAD;
}

static void virq_restore_systick_ns(void)
{
    SysTick_NS->LOAD = g_virq_state_systick_ns.periph.LOAD;
    SysTick_NS->VAL = 0;
    SysTick_NS->CTRL = g_virq_state_systick_ns.periph.CTRL;
}

void virq_acl_add(uint8_t box_id, void * function, uint32_t irqn)
{
    /* Basic checks */
    virq_check_acls(irqn, box_id);

    /* Assign IRQ owneship. */
    g_virq_states[irqn].box_id = box_id;
}

/* virq_acl_add implements vmpu_acl_irq. */
void vmpu_acl_irq(uint8_t box_id, uint32_t irqn) UVISOR_LINKTO(virq_acl_add);

bool virq_is_box_in_active_state(uint8_t box_id)
{
    return g_virq_box_in_active_irq[box_id];
}

void virq_switch(uint8_t src_id, uint8_t dst_id)
{
    bool src_box_in_active_irq = false;

    for (uint32_t irqn = 0; irqn < NVIC_VECTORS; ++irqn) {
        TVirqState * irq = &g_virq_states[irqn];

        /* Put all the source box IRQs on hold.
         * Putting an IRQ on hold means:
         *   - Promote it to secure state, so that NS code cannot modify it.
         *   - De-prioritize it, so that the destination box can be pre-empted.
         */
        if (irq->box_id == src_id) {
            irq->enabled = TZ_NVIC_GetEnableIRQ_NS(irqn);
            irq->priority = TZ_NVIC_GetPriority_NS(irqn);
            if (TZ_NVIC_GetActive_NS(irqn)) {
                src_box_in_active_irq = true;
            }
            assert(irq->priority < g_virq_min_priority);
            TZ_NVIC_SetPriority_NS(irqn, g_virq_min_priority);
            TZ_NVIC_DisableIRQ_NS(irqn);
            NVIC_ClearTargetState(irqn);
        }

        /* Re-enable all the destination box IRQs. */
        if (irq->box_id == dst_id) {
            NVIC_SetTargetState(irqn);
            if (irq->enabled) {
                TZ_NVIC_EnableIRQ_NS(irqn);
            }
            TZ_NVIC_SetPriority_NS(irqn, irq->priority);
        }
    }

    /* Special treatment for SysTick, which is not a user IRQ. */
    /* Note: We assume that box 0 has exclusive owneship of the SysTick, so we
     *       do not virtualize it. In addition, our approach produces a slightly
     *       distorted vision of time. The timer is always reset when
     *       re-entering box 0. */
    if (src_id == 0) {
        virq_copy_systick_ns();
        virq_disable_systick_ns();
        g_virq_state_systick_ns.priority = TZ_NVIC_GetPriority_NS(SysTick_IRQn);
        g_virq_state_systick_ns.active = virq_get_active_systick_ns();
        if (g_virq_state_systick_ns.active) {
            src_box_in_active_irq = true;
        }
        virq_set_active_systick_ns(false);
    }
    if (dst_id == 0) {
        TZ_NVIC_SetPriority_NS(SysTick_IRQn, g_virq_state_systick_ns.priority);
        virq_set_active_systick_ns(g_virq_state_systick_ns.active);
        virq_restore_systick_ns();
        virq_enable_systick_ns();
    }

    /* Save the active state for the source box. */
    g_virq_box_in_active_irq[src_id] = src_box_in_active_irq;
}

void virq_init(uint32_t const * const user_vtor)
{
    /* Detect the number of implemented priority bits.
     * The architecture specifies that unused/not implemented bits in the
     * NVIC IP registers read back as 0. */
    __disable_irq();
    uint8_t volatile * prio = (uint8_t volatile *) &(NVIC->IPR[0]);
    uint8_t prio_bits = *prio;
    *prio = 0xFFU;
    g_virq_prio_bits = (uint8_t) __builtin_popcount(*prio);
    *prio = prio_bits;
    __enable_irq();

    /* Verify that the priority bits read at runtime are realistic. */
    assert(g_virq_prio_bits > 0 && g_virq_prio_bits <= 8);

    /* Set the minimum IRQ priority. */
    g_virq_min_priority = (uint8_t) ((1 << g_virq_prio_bits) - 1);

    /* At the beginning, all IRQs are dis-owned. */
    for (size_t irqn = 0; irqn < NVIC_VECTORS; ++irqn) {
        g_virq_states[irqn].box_id = UVISOR_BOX_ID_INVALID;
        NVIC_ClearTargetState(irqn);
    }
}
