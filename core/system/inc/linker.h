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
#ifndef __LINKER_H__
#define __LINKER_H__

#include "api/inc/priv_sys_irq_hook_exports.h"

extern uint32_t const __code_end__;
extern uint32_t const __stack_start__;
extern uint32_t const __stack_top__;
extern uint32_t const __stack_end__;

extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t const __data_start_src__;

extern void main_entry(void);
extern void isr_default_sys_handler(void);
extern void isr_default_handler(void);
extern int isr_default(uint32_t isr_id);

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t * mode;

    /* Protected BSS section */
    uint32_t * bss_start;
    uint32_t * bss_end;

    /* uVisor own BSS section */
    uint32_t * bss_main_start;
    uint32_t * bss_main_end;

    /* Seecure boxes BSS section */
    uint32_t * bss_boxes_start;
    uint32_t * bss_boxes_end;

    /* uVisor own code and data */
    uint32_t * main_start;
    uint32_t * main_end;

    /* Protected flash memory region */
    uint32_t * secure_start;
    uint32_t * secure_end;

    /* Secure boxes configuration tables */
    uint32_t * cfgtbl_start;
    uint32_t * cfgtbl_end;

    /* Pointers to the secure boxes configuration tables */
    uint32_t * cfgtbl_ptr_start;
    uint32_t * cfgtbl_ptr_end;

    /* Pointers to the secure boxes register gateways */
    uint32_t * register_gateway_ptr_start;
    uint32_t * register_gateway_ptr_end;

    /* Address of __uvisor_box_context */
    uint32_t * * uvisor_box_context;

    /* Main heap for box 0 */
    uint32_t * heap_start;
    uint32_t * heap_end;

    /* Page allocator region */
    uint32_t * page_start;
    uint32_t * page_end;

    /* Page size for the page allocator */
    uint32_t * page_size;

    /* Physical memory boundaries */
    uint32_t * flash_start;
    uint32_t * flash_end;
    uint32_t * sram_start;
    uint32_t * sram_end;

    /* Privileged system IRQ hooks */
    UvisorPrivSystemIRQHooks const * const priv_sys_irq_hooks;

    /* uVisor Lib Functions for use by uVisor */
    void (*box_main_handler)(
        void (*function)(void const *),
        int32_t priority,
        uint32_t stack_pointer,
        uint32_t stack_size);
} UVISOR_PACKED UvisorConfig;

extern UvisorConfig const __uvisor_config;

#endif /* __LINKER_H__ */
