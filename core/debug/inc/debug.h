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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <uvisor.h>
#include "halt.h"
#include "api/inc/debug_exports.h"

/* Debug box handle.
 * This is used internally by uVisor to keep track of the registered debug box
 * and to access its driver when needed. */
typedef struct TDebugBox {
    const TUvisorDebugDriver *driver;
    int initialized;
    uint8_t box_id;
} TDebugBox;
TDebugBox g_debug_box;

void debug_init(void);
void debug_mpu_config(void);
void debug_fault(THaltError reason, uint32_t lr, uint32_t sp);
void debug_map_addr_to_periph(uint32_t address);

/* Debug box */
void debug_register_driver(const TUvisorDebugDriver * const driver);
uint32_t debug_get_version(void);
void debug_halt_error(THaltError reason);
void debug_reboot(TResetReason reason);

#ifdef  NDEBUG

#define DEBUG_INIT(...)          {}
#define DEBUG_FAULT(...)         {}

#else /*NDEBUG*/

#define DEBUG_INIT          debug_init
#define DEBUG_FAULT         debug_fault

#endif/*NDEBUG*/

#endif/*__DEBUG_H__*/
