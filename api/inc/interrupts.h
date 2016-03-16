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
#ifndef __UVISOR_LIB_INTERRUPTS_H__
#define __UVISOR_LIB_INTERRUPTS_H__

UVISOR_EXTERN void vIRQ_SetVectorX(uint32_t irqn, uint32_t vector, uint32_t flag);
UVISOR_EXTERN void vIRQ_SetVector(uint32_t irqn, uint32_t vector);
UVISOR_EXTERN uint32_t vIRQ_GetVector(uint32_t irqn);
UVISOR_EXTERN void vIRQ_EnableIRQ(uint32_t irqn);
UVISOR_EXTERN void vIRQ_DisableIRQ(uint32_t irqn);
UVISOR_EXTERN void vIRQ_ClearPendingIRQ(uint32_t irqn);
UVISOR_EXTERN void vIRQ_SetPendingIRQ(uint32_t irqn);
UVISOR_EXTERN uint32_t vIRQ_GetPendingIRQ(uint32_t irqn);
UVISOR_EXTERN void vIRQ_SetPriority(uint32_t irqn, uint32_t priority);
UVISOR_EXTERN uint32_t vIRQ_GetPriority(uint32_t irqn);
UVISOR_EXTERN int vIRQ_GetLevel(void);

#endif /* __UVISOR_LIB_INTERRUPTS_H__ */
