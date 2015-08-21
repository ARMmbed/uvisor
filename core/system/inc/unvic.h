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

#define UNVIC_MIN_PRIORITY (uint32_t) 1

#define IRQn_OFFSET            16
#define ISR_VECTORS            ((IRQn_OFFSET) + (HW_IRQ_VECTORS))

typedef void (*TIsrVector)(void);

typedef struct {
    TIsrVector hdlr;
    uint8_t    id;
} TIsrUVector;

/* defined in system-specific system.h */
extern const TIsrVector g_isr_vector[ISR_VECTORS];
/* unprivileged interrupts */
extern TIsrUVector g_unvic_vector[HW_IRQ_VECTORS];

extern void     unvic_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag);
extern uint32_t unvic_isr_get(uint32_t irqn);

extern void     unvic_irq_enable(uint32_t irqn);
extern void     unvic_irq_disable(uint32_t irqn);
extern void     unvic_irq_pending_clr(uint32_t irqn);
extern void     unvic_irq_pending_set(uint32_t irqn);
extern uint32_t unvic_irq_pending_get(uint32_t irqn);
extern void     unvic_irq_priority_set(uint32_t irqn, uint32_t priority);
extern uint32_t unvic_irq_priority_get(uint32_t irqn);
extern int      unvic_default(uint32_t isr_id);

extern void unvic_init(void);

static inline __attribute__((always_inline)) void unvic_isr_mux(void)
{
    /* handle IRQ with an unprivileged handler serving an IRQn in un-privileged
     * mode is achieved by mean of two SVCalls; the first one de-previliges
     * execution, the second one re-privileges it note: NONBASETHRDENA (in SCB)
     * must be set to 1 for this to work */
    asm volatile(
        "svc  %[unvic_in]\n"
        "svc  %[unvic_out]\n"
        "bx   lr\n"
        ::[unvic_in]  "i" (UVISOR_SVC_ID_UNVIC_IN),
          [unvic_out] "i" (UVISOR_SVC_ID_UNVIC_OUT)
    );
}

#endif/*__UNVIC_H__*/
