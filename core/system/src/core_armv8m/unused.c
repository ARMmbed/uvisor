/*
 * Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
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
#include <stdint.h>

void unused_v8m_ignore(void)
{
}

void unused_v8m_halt(void)
{
    HALT_ERROR(NOT_IMPLEMENTED, "You called a function that is not implemented for ARMv8-M targets.\n");
}

void     UVISOR_ALIAS(unused_v8m_ignore) boxes_init(void);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_isr_set(uint32_t irqn, uint32_t handler);
uint32_t UVISOR_ALIAS(unused_v8m_halt)   virq_isr_get(uint32_t irqn);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_enable(uint32_t irqn);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_disable(uint32_t irqn);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_pending_clr(uint32_t irqn);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_pending_set(uint32_t irqn);
uint32_t UVISOR_ALIAS(unused_v8m_halt)   virq_irq_pending_get(uint32_t irqn);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_priority_set(uint32_t irqn, uint32_t priority);
uint32_t UVISOR_ALIAS(unused_v8m_halt)   virq_irq_priority_get(uint32_t irqn);
int      UVISOR_ALIAS(unused_v8m_halt)   virq_irq_level_get(void);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_disable_all(void);
void     UVISOR_ALIAS(unused_v8m_halt)   virq_irq_enable_all(void);
