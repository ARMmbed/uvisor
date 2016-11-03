/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#ifndef __UVISOR_API_NVIC_VIRTUAL_H__
#define __UVISOR_API_NVIC_VIRTUAL_H__

#include "api/inc/interrupts.h"
#include "api/inc/unvic_exports.h"

/** @addtogroup interrupt
 * @{
 */

#define NVIC_SetPriorityGrouping    __NVIC_SetPriorityGrouping                  /**< @showinitializer */
#define NVIC_GetPriorityGrouping    __NVIC_GetPriorityGrouping                  /**< @showinitializer */
#define NVIC_EnableIRQ              vIRQ_EnableIRQ                              /**< @showinitializer */
#define NVIC_DisableIRQ             vIRQ_DisableIRQ                             /**< @showinitializer */
#define NVIC_GetPendingIRQ          vIRQ_GetPendingIRQ                          /**< @showinitializer */
#define NVIC_SetPendingIRQ          vIRQ_SetPendingIRQ                          /**< @showinitializer */
#define NVIC_ClearPendingIRQ        vIRQ_ClearPendingIRQ                        /**< @showinitializer */
#define NVIC_GetActive              __NVIC_GetActive                            /**< @showinitializer */
#define NVIC_SetPriority            vIRQ_SetPriority                            /**< @showinitializer */
#define NVIC_GetPriority            vIRQ_GetPriority                            /**< @showinitializer */
#define NVIC_SystemReset()          vIRQ_SystemReset(RESET_REASON_NO_REASON)    /**< @showinitializer */

/** @} */

#endif /* __UVISOR_API_NVIC_VIRTUAL_H__ */
