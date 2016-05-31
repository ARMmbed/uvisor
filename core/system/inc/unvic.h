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

#ifndef __UNVIC_H__
#define __UNVIC_H__

#include "svc.h"
#include "api/inc/unvic_exports.h"

#define UNVIC_IS_IRQ_ENABLED(irqn) (NVIC->ISER[(((uint32_t) ((int32_t) (irqn))) >> 5UL)] & \
                                    (uint32_t) (1UL << (((uint32_t) ((int32_t) (irqn))) & 0x1FUL)))

#define ISR_VECTORS ((NVIC_OFFSET) + (NVIC_VECTORS))

typedef void (*TIsrVector)(void);

typedef struct {
    TIsrVector hdlr;        /**< Unprivileged ISR tied to the IRQn slot. 0 if the slot is unregistered. */
    bool       was_enabled; /**< State kept across a virtualized __disable_irq()/__enable_irq() function call. */
    uint8_t    id;          /**< Box ID of the IRQn owner. If the handler is set to 0 this field has no meaning. */
} TIsrUVector;

/* defined in system-specific system.h */
extern const TIsrVector g_isr_vector[ISR_VECTORS];
/* unprivileged interrupts */
extern TIsrUVector g_unvic_vector[NVIC_VECTORS];

extern void     unvic_isr_set(uint32_t irqn, uint32_t vector);
extern uint32_t unvic_isr_get(uint32_t irqn);

extern void     unvic_irq_enable(uint32_t irqn);
extern void     unvic_irq_disable(uint32_t irqn);
extern void     unvic_irq_pending_clr(uint32_t irqn);
extern void     unvic_irq_pending_set(uint32_t irqn);
extern uint32_t unvic_irq_pending_get(uint32_t irqn);
extern void     unvic_irq_priority_set(uint32_t irqn, uint32_t priority);
extern uint32_t unvic_irq_priority_get(uint32_t irqn);
extern int      unvic_irq_level_get(void);
extern int      unvic_default(uint32_t isr_id);

extern void unvic_init(void);

/** Perform a context switch-in as a result of an interrupt request.
 *
 * This function uses information from an SVCall to retrieve an interrupt
 * handler, validate it, and use its metadata to perform the context switch. It
 * should not be called directly by new code, as it is mapped to the hardcoded
 * table of SVCall handlers in uVisor.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway
 * @param svc_pc[in]    Program counter at the time of the secure gateway */
void UVISOR_NAKED unvic_gateway_in(uint32_t svc_sp, uint32_t svc_pc);

/** Perform a context switch-out as a result of an interrupt request.
 *
 * This function uses information from an SVCall to return from a previously
 * switched-in unprivileged interrupt handler. It should not be called directly
 * by new code, as it is mapped to the hardcoded table of SVCall handlers in
 * uVisor.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway return handler (thunk) */
void UVISOR_NAKED unvic_gateway_out(uint32_t svc_sp);

/** Disable all interrupts for the currently active box.
 *
 * This function selectively disables all interrupts that belong to the current
 * box and keeps a state on whether they were enabled before this operation. The
 * state is then used when re-enabiling the interrupts later on, in
 * ::unvic_irq_enable_all. */
extern void unvic_irq_disable_all(void);

/** Re-enable all previously interrupts for the currently active box.
 *
 * This function re-enables all interrupts that belong to the current box and
 * that were previously disabled with the ::unvic_irq_disable_all function. */
extern void unvic_irq_enable_all(void);

#endif/*__UNVIC_H__*/
