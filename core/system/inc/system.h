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
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <uvisor.h>
#include "unvic.h"

/* Default vector table (placed in Flash) */
extern const TIsrVector g_isr_vector[ISR_VECTORS];

/* Default system hooks (placed in SRAM) */
extern UvisorPrivSystemHooks g_priv_sys_hooks;

/* Default ISRs prototypes */
void isr_default_sys_handler(void);
void isr_default_handler(void);

/* System IRQs */
extern void NonMaskableInt_IRQn_Handler(void);
extern void HardFault_IRQn_Handler(void);
extern void MemoryManagement_IRQn_Handler(void);
extern void BusFault_IRQn_Handler(void);
extern void UsageFault_IRQn_Handler(void);
extern void SVCall_IRQn_Handler(void);
extern void DebugMonitor_IRQn_Handler(void);
extern void PendSV_IRQn_Handler(void);
extern void SysTick_IRQn_Handler(void);

#endif /* __SYSTEM_H__ */
