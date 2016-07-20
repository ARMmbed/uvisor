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
#ifndef __REGISTER_GATEWAY_H__
#define __REGISTER_GATEWAY_H__

#include "api/inc/register_gateway_exports.h"
#include <stdint.h>

/** Perform a register gateway operation.
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the register
 *                      gateway
 * @param svc_pc[in]    Program counter at the time of the register gateway
 */
void register_gateway_perform_operation(uint32_t svc_sp, uint32_t svc_pc);

#endif /* __REGISTER_GATEWAY_H__ */
