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
#ifndef __LINKER_H__
#define __LINKER_H__

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

    /* initialized secret data section */
    uint32_t *cfgtbl_start, *cfgtbl_end;

    /* initialized secret data section */
    uint32_t *data_src, *data_start, *data_end;

    /* uninitialized secret data section */
    uint32_t *bss_start, *bss_end;

    /* uvisors protected flash memory regions */
    uint32_t *secure_start, *secure_end;

    /* memory reserved for uvisor */
    uint32_t *reserved_start, *reserved_end;

    /* address of __uvisor_box_context */
    uint32_t **uvisor_box_context;
} UVISOR_PACKED UvisorConfig;

UVISOR_EXTERN const UvisorConfig __uvisor_config;

#endif/*__LINKER_H__*/
