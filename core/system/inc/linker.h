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

#include "api/inc/priv_sys_hooks_exports.h"
#include "api/inc/lib_hook_exports.h"
#include "api/inc/debug_exports.h"
#include "api/inc/linker_exports.h"

extern uint32_t const __uvisor_code_end__;
extern uint32_t const __uvisor_stack_start__;
extern uint32_t const __uvisor_stack_top__;
extern uint32_t const __uvisor_stack_end__;
extern uint32_t const __uvisor_stack_start_np__;
extern uint32_t const __uvisor_stack_top_np__;
extern uint32_t const __uvisor_stack_end_np__;

extern uint32_t __uvisor_bss_start__;
extern uint32_t __uvisor_bss_end__;

extern uint32_t __uvisor_data_start__;
extern uint32_t __uvisor_data_end__;
extern uint32_t const __uvisor_data_start_src__;

extern uint32_t const __uvisor_entry_points_start__;
extern uint32_t const __uvisor_entry_points_end__;

extern void main_entry(uint32_t caller);
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

    /* Public heap (used by the public box) */
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

    /* Physical memory boundaries of the public SRAM, the one where the public
     * box and the page heap space is. When a TCM is available, what uVisor
     * regards as "SRAM" is the memory where its own memories and the
     * compile-time defined memories for the private boxes are. */
    uint32_t * public_sram_start;
    uint32_t * public_sram_end;

    /* Newlib impure pointer */
    uint32_t * * newlib_impure_ptr;

    /* Privileged system hooks */
    UvisorPrivSystemHooks const * const priv_sys_hooks;

    /* Functions provided by uVisor Lib for use by uVisor in unprivileged mode
     * */
    UvisorLibHooks const * const lib_hooks;

    TUvisorDebugDriver const * const debug_driver;
} UVISOR_PACKED UvisorConfig;

extern UvisorConfig const __uvisor_config;

#endif /* __LINKER_H__ */
