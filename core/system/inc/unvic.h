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
#include "unvic_exports.h"

#define UNVIC_IS_IRQ_ENABLED(irqn) (NVIC->ISER[(((uint32_t) ((int32_t) (irqn))) >> 5UL)] & \
                                    (uint32_t) (1UL << (((uint32_t) ((int32_t) (irqn))) & 0x1FUL)))

#define NVIC_OFFSET 16
#define ISR_VECTORS ((NVIC_OFFSET) + (NVIC_VECTORS))

typedef void (*TIsrVector)(void);

typedef struct {
    TIsrVector hdlr;
    uint8_t    id;
} TIsrUVector;

/* defined in system-specific system.h */
extern const TIsrVector g_isr_vector[ISR_VECTORS];
/* unprivileged interrupts */
extern TIsrUVector g_unvic_vector[NVIC_VECTORS];

extern void     unvic_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag);
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

#endif/*__UNVIC_H__*/
