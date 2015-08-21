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

void debug_init(void);
void debug_cx_switch_in(void);
void debug_cx_switch_out(void);
void debug_exc_sf(uint32_t lr);
void debug_fault_memmanage(void);
void debug_fault_bus(void);
void debug_fault_usage(void);
void debug_fault_hard(void);
void debug_fault_debug(void);
void debug_fault(THaltError reason, uint32_t lr);
void debug_mpu_config(void);
void debug_mpu_fault(void);
void debug_map_addr_to_periph(uint32_t address);

#define DEBUG_PRINT_HEAD(x) {\
    DPRINTF("\n***********************************************************\n");\
    DPRINTF("                    "x"\n");\
    DPRINTF("***********************************************************\n\n");\
}
#define DEBUG_PRINT_END()   {\
    DPRINTF("***********************************************************\n\n");\
}

#ifdef  NDEBUG

#define DEBUG_INIT(...)          {}
#define DEBUG_FAULT(...)         {}
#define DEBUG_CX_SWITCH_IN(...)  {}
#define DEBUG_CX_SWITCH_OUT(...) {}

#else /*NDEBUG*/

#define DEBUG_INIT          debug_init
#define DEBUG_FAULT         debug_fault
#define DEBUG_CX_SWITCH_IN  debug_cx_switch_in
#define DEBUG_CX_SWITCH_OUT debug_cx_switch_out

#endif/*NDEBUG*/

#endif/*__DEBUG_H__*/
