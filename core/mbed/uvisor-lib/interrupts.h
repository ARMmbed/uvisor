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
#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

UVISOR_EXTERN void     __uvisor_isr_set(uint32_t irqn, uint32_t vector,
                                        uint32_t flag);
UVISOR_EXTERN uint32_t __uvisor_isr_get(uint32_t irqn);

UVISOR_EXTERN void     __uvisor_irq_enable(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_disable(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_pending_clr(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_pending_set(uint32_t irqn);
UVISOR_EXTERN uint32_t __uvisor_irq_pending_get(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_priority_set(uint32_t irqn,
                                                 uint32_t priority);
UVISOR_EXTERN uint32_t __uvisor_irq_priority_get(uint32_t irqn);

#define vIRQ_SetVectorX(irqn, vector, flag)                                    \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetVector((IRQn_Type) (irqn), (uint32_t) (vector));           \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_isr_set((uint32_t) (irqn), (uint32_t) (vector),           \
                             (uint32_t) (flag));                               \
        }                                                                      \
    })

#define vIRQ_SetVector(irqn, vector) vIRQ_SetVectorX(irqn, vector, 0)

#define vIRQ_GetVector(irqn)                                                   \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetVector((IRQn_Type) (irqn));                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res = __uvisor_isr_get((uint32_t) (irqn));                         \
        }                                                                      \
        res;                                                                   \
    })

#define vIRQ_EnableIRQ(irqn)                                                   \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_EnableIRQ((IRQn_Type) (irqn));                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_enable((uint32_t) (irqn));                            \
        }                                                                      \
    })

#define vIRQ_DisableIRQ(irqn)                                                  \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_DisableIRQ((IRQn_Type) (irqn));                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_disable((uint32_t) (irqn));                           \
        }                                                                      \
    })

#define vIRQ_ClearPendingIRQ(irqn)                                             \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_ClearPendingIRQ((IRQn_Type) (irqn));                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_pending_clr((uint32_t) (irqn));                       \
        }                                                                      \
    })

#define vIRQ_SetPendingIRQ(irqn)                                               \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetPendingIRQ((IRQn_Type) (irqn));                            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_pending_set((uint32_t) (irqn));                       \
        }                                                                      \
    })

#define vIRQ_GetPendingIRQ(irqn)                                               \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetPendingIRQ((IRQn_Type) (irqn));                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res =  __uvisor_irq_pending_get((uint32_t) (irqn));                \
        }                                                                      \
        res;                                                                   \
    })

#define vIRQ_SetPriority(irqn, priority)                                       \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetPriority((IRQn_Type) (irqn), (uint32_t) (priority));       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_priority_set((uint32_t) (irqn),                       \
                                      (uint32_t) (priority));                  \
        }                                                                      \
    })

#define vIRQ_GetPriority(irqn)                                                 \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetPriority((IRQn_Type) (irqn));                        \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res =  __uvisor_irq_priority_get((uint32_t) (irqn));               \
        }                                                                      \
        res;                                                                   \
    })

#endif/*__INTERRUPTS_H__*/
