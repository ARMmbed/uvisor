/*
 * Copyright (c) 2015-2015, ARM Limited, All Rights Reserved
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

/* note: only include this header file if you need to use code in which you do not
 * want to rename NVIC APIs into vIRQ APIs; it must be included directly in the
 * source file and as the last header file; inclusion is protected by a guard which
 * is dependent on the YOTTA_CFG_UVISOR_PRESENT symbol (set in the target) and the
 * UVISOR_NO_HOOKS symbol (defined by uVisor) */

#ifndef __UVISOR_LIB_OVERRIDE_H__
#define __UVISOR_LIB_OVERRIDE_H__

#include <stdint.h>

extern void vIRQ_SetVectorX(uint32_t irqn, uint32_t vector, uint32_t flag);
extern void vIRQ_SetVector(uint32_t irqn, uint32_t vector);
extern uint32_t vIRQ_GetVector(uint32_t irqn);
extern void vIRQ_EnableIRQ(uint32_t irqn);
extern void vIRQ_DisableIRQ(uint32_t irqn);
extern void vIRQ_ClearPendingIRQ(uint32_t irqn);
extern void vIRQ_SetPendingIRQ(uint32_t irqn);
extern uint32_t vIRQ_GetPendingIRQ(uint32_t irqn);
extern void vIRQ_SetPriority(uint32_t irqn, uint32_t priority);
extern uint32_t vIRQ_GetPriority(uint32_t irqn);

#if YOTTA_CFG_UVISOR_PRESENT == 1 && !defined(UVISOR_NO_HOOKS)

/* re-definition of NVIC APIs supported by uVisor */
#define NVIC_ClearPendingIRQ(irqn)       vIRQ_ClearPendingIRQ((uint32_t) (irqn))
#define NVIC_SetPendingIRQ(irqn)         vIRQ_SetPendingIRQ((uint32_t) (irqn))
#define NVIC_GetPendingIRQ(irqn)         vIRQ_GetPendingIRQ((uint32_t) (irqn))
#define NVIC_SetPriority(irqn, priority) vIRQ_SetPriority((uint32_t) (irqn), (uint32_t) (priority))
#define NVIC_GetPriority(irqn)           vIRQ_GetPriority((uint32_t) (irqn))
#define NVIC_SetVector(irqn, vector)     vIRQ_SetVector((uint32_t) (irqn), (uint32_t) (vector))
#define NVIC_GetVector(irqn)             vIRQ_GetVector((uint32_t) (irqn))
#define NVIC_EnableIRQ(irqn)             vIRQ_EnableIRQ((uint32_t) (irqn))
#define NVIC_DisableIRQ(irqn)            vIRQ_DisableIRQ((uint32_t) (irqn))

#endif /* YOTTA_CFG_UVISOR_PRESENT == 1 && !defined(UVISOR_NO_HOOKS) */

#endif /* __UVISOR_LIB_OVERRIDE_H__ */
