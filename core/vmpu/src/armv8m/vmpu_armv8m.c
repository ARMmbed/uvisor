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
#include "vmpu.h"

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

void vmpu_acl_sram(uint8_t box_id, uint32_t bss_size, uint32_t stack_size, uint32_t * bss_start,
                   uint32_t * stack_pointer)
{
    /* FIXME: The non-public box behavior of this function must be implemented
     *        for the vMPU port to be complete. */
}

void vmpu_arch_init(void)
{
    /* AIRCR needs to be unlocked with this key on every write. */
    const uint32_t SCB_AIRCR_VECTKEY = 0x5FAUL;

    /* AIRCR configurations:
     *      - Non-secure exceptions are de-prioritized.
     *      - BusFault, HardFault, and NMI are Secure.
     */
    /* TODO: Setup a sensible priority grouping. */
    uint32_t aircr = SCB->AIRCR;
    SCB->AIRCR = (SCB_AIRCR_VECTKEY << SCB_AIRCR_VECTKEY_Pos) |
                 (aircr & SCB_AIRCR_ENDIANESS_Msk) |   /* Keep unchanged */
                 (SCB_AIRCR_PRIS_Msk) |
                 (0 << SCB_AIRCR_BFHFNMINS_Pos) |
                 (aircr & SCB_AIRCR_PRIGROUP_Msk) |    /* Keep unchanged */
                 (0 << SCB_AIRCR_SYSRESETREQ_Pos) |
                 (0 << SCB_AIRCR_VECTCLRACTIVE_Pos);

    /* SHCSR configurations:
     *      - SecureFault exception enabled.
     *      - UsageFault exception enabled for the selected Security state.
     *      - BusFault exception enabled.
     *      - MemManage exception enabled for the selected Security state.
     */
    SCB->SHCSR |= (SCB_SHCSR_SECUREFAULTENA_Msk) |
                  (SCB_SHCSR_USGFAULTENA_Msk) |
                  (SCB_SHCSR_BUSFAULTENA_Msk) |
                  (SCB_SHCSR_MEMFAULTENA_Msk);

    /* Initialize static SAU regions. */
    vmpu_arch_init_hw();
}

void vmpu_arch_init_hw(void)
{
    /* FIXME: This function must use the vMPU APIs to be complete. */

    /* Disable the SAU and configure all code to run in S mode. */
    SAU->CTRL = 0;
    uint32_t rnr = 0;

    /* Configure User interrupt table in NS mode. */
    SAU->RNR = rnr++;
    SAU->RBAR = ((uint32_t) __uvisor_config.flash_start) & 0xFFFFFFE0;
    SAU->RLAR = (((uint32_t) &__uvisor_entry_points_start__ - 31) & 0xFFFFFFE0) | SAU_RLAR_ENABLE_Msk;

    /* Configure the gateways in NSC mode. */
    SAU->RNR = rnr++;
    SAU->RBAR = ((uint32_t) &__uvisor_entry_points_start__) & 0xFFFFFFE0;
    SAU->RLAR = (((uint32_t) &__uvisor_entry_points_end__ - 31) & 0xFFFFFFE0) | SAU_RLAR_NSC_Msk | SAU_RLAR_ENABLE_Msk;

    /* Configure the remainder of the flash to be accessible in NS mode. */
    SAU->RNR = rnr++;
    SAU->RBAR = ((uint32_t) &__uvisor_entry_points_end__) & 0xFFFFFFE0;
    SAU->RLAR = (((uint32_t) __uvisor_config.flash_end - 31) & 0xFFFFFFE0) | SAU_RLAR_ENABLE_Msk;

    /* Configure the public SRAM to be accessible in NS mode. */
    SAU->RNR = rnr++;
    SAU->RBAR = ((uint32_t) __uvisor_config.page_end) & 0xFFFFFFE0;
    SAU->RLAR = (((uint32_t) __uvisor_config.sram_end - 31) & 0xFFFFFFE0) | SAU_RLAR_ENABLE_Msk;

    /* Configure the public peripherals to be accessible in NS mode. */
    SAU->RNR = rnr++;
    SAU->RBAR = ((uint32_t) 0x40000000) & 0xFFFFFFE0;
    SAU->RLAR = (((uint32_t) 0x50000000 - 31) & 0xFFFFFFE0) | SAU_RLAR_ENABLE_Msk;

    /* Enable the SAU. */
    SAU->CTRL |= SAU_CTRL_ENABLE_Msk;

}

void vmpu_order_boxes(int * const best_order, int box_count)
{
    for (int i = 0; i < box_count; ++i) {
        best_order[i] = i;
    }
}
