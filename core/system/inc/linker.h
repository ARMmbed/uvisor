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

UVISOR_EXTERN const uint32_t __code_end__;
UVISOR_EXTERN const uint32_t __stack_start__;
UVISOR_EXTERN const uint32_t __stack_top__;
UVISOR_EXTERN const uint32_t __stack_end__;

UVISOR_EXTERN uint32_t __bss_start__;
UVISOR_EXTERN uint32_t __bss_end__;

UVISOR_EXTERN uint32_t __data_start__;
UVISOR_EXTERN uint32_t __data_end__;
UVISOR_EXTERN const uint32_t __data_start_src__;

UVISOR_EXTERN void main_entry(void);
UVISOR_EXTERN void isr_default_sys_handler(void);
UVISOR_EXTERN void isr_default_handler(void);
UVISOR_EXTERN int isr_default(uint32_t isr_id);

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t *mode;

    /* protected bss */
    uint32_t *bss_start, *bss_end;

        /* uvisor main bss */
    uint32_t *bss_main_start, *bss_main_end;

        /* secure boxes bss */
    uint32_t *bss_boxes_start, *bss_boxes_end;

    /* uvisor code and data */
    uint32_t *main_start, *main_end;

    /* protected flash memory region */
    uint32_t *secure_start, *secure_end;

        /* boxes configuration tables */
    uint32_t *cfgtbl_start, *cfgtbl_end;

        /* pointers to boxes configuration tables */
    uint32_t *cfgtbl_ptr_start, *cfgtbl_ptr_end;

    /* address of __uvisor_box_context */
    uint32_t **uvisor_box_context;

    /* Heap start and end addresses for box zero */
    uint32_t * heap_start;
    uint32_t * heap_end;

    /* Page allocator start and end addresses */
    uint32_t * page_start;
    uint32_t * page_end;
    /* Pointer to page size used for page allocator */
    uint32_t * page_size;

    /* Physical memories' boundaries */
    uint32_t *flash_start;
    uint32_t *flash_end;
    uint32_t *sram_start;
    uint32_t *sram_end;

    /* Privileged system IRQ hooks */
    const UvisorPrivSystemIRQHooks * const priv_sys_irq_hooks;
} UVISOR_PACKED UvisorConfig;

UVISOR_EXTERN const UvisorConfig __uvisor_config;

#endif/*__LINKER_H__*/
