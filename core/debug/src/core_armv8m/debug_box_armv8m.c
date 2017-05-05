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
#include "debug.h"
#include "context.h"
#include "linker.h"
#include "secure_transitions.h"

void debug_die(void)
{
    while (1);
}

/* Note: On ARMv8-M the return_handler is executed in S mode. */
void debug_deprivilege_and_return(void * debug_handler, void * return_handler,
                                  uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    /* Switch to the debug box.
     * We use a regular process switch, so we don't need a dedicated stack for
     * the debug box. */
    uint8_t box_id = g_debug_box.box_id;
    context_switch_in(CONTEXT_SWITCH_UNBOUND_THREAD, box_id, 0, 0);

    /* De-privilege, call the debug box handler, re-privilege, call the return
     * handler. */
    uint32_t caller = UVISOR_GET_NS_ALIAS(UVISOR_GET_NS_ADDRESS((uint32_t) debug_handler));
    SECURE_TRANSITION_S_TO_NS(caller, a0, a1, a2, a3);
    ((void (*)(void)) return_handler)();
}
