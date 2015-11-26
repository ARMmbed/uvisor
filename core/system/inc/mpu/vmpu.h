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
#ifndef __VMPU_H__
#define __VMPU_H__

#include "vmpu_exports.h"
#include "vmpu_unpriv_access.h"

#define VMPU_FLASH_ADDR_MASK  (~(((uint32_t)(FLASH_LENGTH)) - 1))
#define VMPU_FLASH_ADDR(addr) ((VMPU_FLASH_ADDR_MASK & (uint32_t)(addr)) == \
                               (FLASH_ORIGIN & VMPU_FLASH_ADDR_MASK))

#define VMPU_REGION_SIZE(p1, p2) ((p1 >= p2) ? 0 : \
                                             ((uint32_t) (p2) - (uint32_t) (p1)))

#define VMPU_SCB_BFSR  (*((volatile uint8_t *) &SCB->CFSR + 1))
#define VMPU_SCB_MMFSR (*((volatile uint8_t *) &SCB->CFSR))

/* opcode encoding for ldr/str instructions
 *
 * for str instructions we expect the following:
 *   Operand | Purpose | Symbol | Value
 *   ----------------------------------
 *   imm5    | offset  | none   | 0x0
 *   Rn      | addr    | r0     | 0x0
 *   Rt      | src     | r1     | 0x1
 *
 * for ldr instructions we expect the following:
 *   Operand | Purpose | Symbol | Value
 *   ----------------------------------
 *   imm5    | offset  | none   | 0x0
 *   Rn      | addr    | r0     | 0x0
 *   Rt      | dst     | r0     | 0x0
 *
 *   |-------------|---------------------------------------------|--------|
 *   |             | Opcode base    | imm5       | Rn    | Rt    |        |
 *   | instruction |---------------------------------------------| Opcode |
 *   |             | 15 14 13 12 11 | 10 9 8 7 6 | 5 4 3 | 2 1 0 |        |
 *   |-------------|---------------------------------------------|--------|
 *   | str         |  0  1  1  0  0 |  0 0 0 0 0 | 0 0 0 | 0 0 1 | 0x6001 |
 *   | strh        |  1  0  0  0  0 |  0 0 0 0 0 | 0 0 0 | 0 0 1 | 0x8001 |
 *   | strb        |  0  1  1  1  0 |  0 0 0 0 0 | 0 0 0 | 0 0 1 | 0x7001 |
 *   | ldr         |  0  1  1  0  1 |  0 0 0 0 0 | 0 0 0 | 0 0 0 | 0x6800 |
 *   | ldrh        |  1  0  0  0  1 |  0 0 0 0 0 | 0 0 0 | 0 0 0 | 0x8800 |
 *   | ldrb        |  0  1  1  1  1 |  0 0 0 0 0 | 0 0 0 | 0 0 0 | 0x7800 |
 *   |-------------|---------------------------------------------|--------|
 *
 */
#define VMPU_OPCODE16_LOWER_R0_R1_MASK  0x01
#define VMPU_OPCODE16_LOWER_R0_R0_MASK  0x00
#define VMPU_OPCODE16_UPPER_STR_MASK    0x60
#define VMPU_OPCODE16_UPPER_STRH_MASK   0x80
#define VMPU_OPCODE16_UPPER_STRB_MASK   0x70
#define VMPU_OPCODE16_UPPER_LDR_MASK    0x68
#define VMPU_OPCODE16_UPPER_LDRH_MASK   0x88
#define VMPU_OPCODE16_UPPER_LDRB_MASK   0x78

/* bit-banding regions boundaries */
#define VMPU_SRAM_START           0x20000000
#define VMPU_SRAM_BITBAND_START   0x22000000
#define VMPU_SRAM_BITBAND_END     0x23FFFFFF
#define VMPU_PERIPH_START         0x40000000
#define VMPU_PERIPH_BITBAND_START 0x42000000
#define VMPU_PERIPH_BITBAND_END   0x43FFFFFF

/* bit-banding aliases macros
 * physical address ---> bit-banded alias
 * bit-banded alias ---> physical address
 * bit-banded alias ---> specific bit accessed at physical address */
#define VMPU_SRAM_BITBAND_ADDR_TO_ALIAS(addr, bit)   (VMPU_SRAM_BITBAND_START + 32U * (((uint32_t) (addr)) - \
                                                     VMPU_SRAM_START) + 4U * ((uint32_t) (bit)))
#define VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(alias)       ((((((uint32_t) (alias)) - VMPU_SRAM_BITBAND_START) >> 5) & \
                                                     ~0x3U) + VMPU_SRAM_START)
#define VMPU_SRAM_BITBAND_ALIAS_TO_BIT(alias)        (((((uint32_t) (alias)) - VMPU_SRAM_BITBAND_START) - \
                                                     ((VMPU_SRAM_BITBAND_ALIAS_TO_ADDR(alias) - \
                                                     VMPU_SRAM_START) << 5)) >> 2)
#define VMPU_PERIPH_BITBAND_ADDR_TO_ALIAS(addr, bit) (VMPU_PERIPH_BITBAND_START + 32U * (((uint32_t) (addr)) - \
                                                     VMPU_PERIPH_START) + 4U * ((uint32_t) (bit)))
#define VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(alias)     ((((((uint32_t) (alias)) - VMPU_PERIPH_BITBAND_START) >> 5) & \
                                                     ~0x3U) + VMPU_PERIPH_START)
#define VMPU_PERIPH_BITBAND_ALIAS_TO_BIT(alias)      (((((uint32_t) (alias)) - VMPU_PERIPH_BITBAND_START) - \
                                                     ((VMPU_PERIPH_BITBAND_ALIAS_TO_ADDR(alias) - \
                                                     VMPU_PERIPH_START) << 5)) >> 2)

extern uint8_t g_active_box;

extern void vmpu_acl_add(uint8_t box_id, void *addr,
                         uint32_t size, UvisorBoxAcl acl);
extern void vmpu_acl_irq(uint8_t box_id, void *function, uint32_t isr_id);
extern int  vmpu_acl_dev(UvisorBoxAcl acl, uint16_t device_id);
extern int  vmpu_acl_mem(UvisorBoxAcl acl, uint32_t addr, uint32_t size);
extern int  vmpu_acl_reg(UvisorBoxAcl acl, uint32_t addr, uint32_t rmask,
                         uint32_t wmask);
extern int  vmpu_acl_bit(UvisorBoxAcl acl, uint32_t addr);

extern void vmpu_switch(uint8_t src_box, uint8_t dst_box);

extern void vmpu_load_box(uint8_t box_id);

extern int vmpu_fault_recovery_bus(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status);

uint32_t vmpu_fault_find_acl(uint32_t fault_addr, uint32_t size);

extern void vmpu_acl_stack(uint8_t box_id, uint32_t context_size, uint32_t stack_size);
extern uint32_t vmpu_acl_static_region(uint8_t region, void* base, uint32_t size, UvisorBoxAcl acl);

extern void vmpu_arch_init(void);
extern void vmpu_arch_init_hw(void);
extern int  vmpu_init_pre(void);
extern void vmpu_init_post(void);

extern void vmpu_sys_mux_handler(uint32_t lr, uint32_t msp);

extern uint32_t  g_vmpu_box_count;

extern uint32_t vmpu_register_gateway(uint32_t addr, uint32_t val);

static inline __attribute__((always_inline)) void vmpu_sys_mux(void)
{
    /* handle IRQ with an unprivileged handler serving an IRQn in un-privileged
     * mode is achieved by mean of two SVCalls; the first one de-previliges
     * execution, the second one re-privileges it note: NONBASETHRDENA (in SCB)
     * must be set to 1 for this to work */
    asm volatile(
        "mov r0, lr\n"
        "mrs r1, MSP\n"
        "push {lr}\n"
        "blx vmpu_sys_mux_handler\n"
        "pop {pc}\n"
    );
}

#endif/*__VMPU_H__*/
