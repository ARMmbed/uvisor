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
#include "mbed/mbed.h"
#include "uvisor-lib/uvisor-lib.h"

void __uvisor_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    register uint32_t __r1 __asm__("r1") = vector;
    register uint32_t __r2 __asm__("r2") = flag;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [r1]     "r" (__r1),
          [r2]     "r" (__r2),
          [svc_id] "i" (UVISOR_SVC_ID_ISR_SET)
    );
}

uint32_t __uvisor_isr_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_ISR_GET)
    );
    return __res;
}

void __uvisor_irq_enable(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_ENABLE)
    );
}

void __uvisor_irq_disable(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_DISABLE)
    );
}

void __uvisor_irq_pending_clr(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PEND_CLR)
    );
}

void __uvisor_irq_pending_set(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PEND_SET)
    );
}

uint32_t __uvisor_irq_pending_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_IRQ_PEND_GET)
    );
    return __res;
}

void __uvisor_irq_priority_set(uint32_t irqn, uint32_t priority)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    register uint32_t __r1 __asm__("r1") = priority;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [r1]     "r" (__r1),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PRIO_SET)
    );
}

uint32_t __uvisor_irq_priority_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_IRQ_PRIO_GET)
    );
    return __res;
}
