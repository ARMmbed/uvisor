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
#include <uvisor.h>
#include "context.h"

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    return 0;
}

uint32_t vmpu_sys_mux_handler(uint32_t lr, uint32_t msp)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
    while(1);
    return 0;
}

void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

void vmpu_load_box(uint8_t box_id)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

void vmpu_acl_stack(uint8_t box_id, uint32_t bss_size, uint32_t stack_size)
{
    /* The public box is handled separately. */
    if (box_id == 0) {
        /* Non-important sanity checks */
        assert(stack_size == 0);

        /* For backwards-compatibility, the public box uses the legacy stack and
         * heap. */
        DPRINTF("[box 0] BSS size: %i. Stack size: %i\r\n", bss_size, stack_size);
        g_context_current_states[0].sp = __get_PSP();
        g_context_current_states[0].bss = (uint32_t) __uvisor_config.heap_start;
        return;
    }

    /* FIXME: The non-public box behavior of this function must be implemented
     *        for the vMPU port to be complete. */
}

void vmpu_arch_init(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}

void vmpu_arch_init_hw(void)
{
    /* FIXME: This function must be implemented for the vMPU port to be complete. */
}
