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
.globl uvisor_init
.globl uvisor_export_table_size
.globl uvisor_config
.globl uvisor_ctx
.type  uvisor_init, %function
.weak  __uvisor_mode
.weak  SVC_Handler
.weak  PendSV_Handler
.weak  SysTick_Handler

.section .uvisor.main, "x"
    .thumb
    .thumb_func

uvisor_init:
    /* uvisor - precompiled blob */
    .incbin UVISOR_BIN

/* The size of the uVisor export table is a uint32_t located at the end of the
 * uVisor binary. The uVisor export table is located at size number of bytes
 * before the end of the uVisor binary. Size includes the 4 bytes for size
 * itself. */
.set uvisor_export_table_size, uvisor_config - 4

uvisor_config:
    /* uvisor expects its configuration section right
       after itself in flash. If the configuration magic is not found,
       uvisor will intentionally freeze to avoid accidentally
       unprotect systems */
    .long UVISOR_MAGIC

    /* MBED_VERSION */
    .long 0

    /* uvisor mode
       0: disabled
       1: permissive
       2: enabled */
    .long __uvisor_mode

    /* start and end address of protected bss */
    .long __uvisor_bss_start
    .long __uvisor_bss_end

    /* start and end address of uvisor main bss */
    .long __uvisor_bss_main_start
    .long __uvisor_bss_main_end

    /* start and end address of uvisor secure boxes bss */
    .long __uvisor_bss_boxes_start
    .long __uvisor_bss_boxes_end

    /* start and end address of uvisor code and data */
    .long __uvisor_main_start
    .long __uvisor_main_end

    /* start and end address of protected flash region
    /* note: remaining flash size is available for configuration storage */
    .long __uvisor_secure_start
    .long __uvisor_secure_end

     /* start and end address of boxes configuration tables */
    .long __uvisor_cfgtbl_start
    .long __uvisor_cfgtbl_end

    /* start and end address of list of pointers to boxes configuration tables */
    .long __uvisor_cfgtbl_ptr_start
    .long __uvisor_cfgtbl_ptr_end

    /* pointer to __uvisor_box_context */
    .long __uvisor_box_context

    /* Physical memories' boundaries */
    .long __uvisor_flash_start;
    .long __uvisor_flash_end;
    .long __uvisor_sram_start;
    .long __uvisor_sram_end;

    /* Privileged handlers */
    /* These have the same name as the CMSIS interrupt handlers on purpose, so
     * we can re-use those already defined handlers and avoid modifying
     * pre-existing applications for uVisor compatibility. */
    .long SVC_Handler
    .long PendSV_Handler
    .long SysTick_Handler

__uvisor_mode:
    /* uvisor default mode - user can override weak reference */
    .long 0

    /* Default privileged handlers */
SVC_Handler:
    .long 0

PendSV_Handler:
    .long 0

SysTick_Handler:
    .long 0

.section .bss
__uvisor_box_context:
uvisor_ctx:
    .long 0

.section .keep.uvisor.bss.main, "awM", @nobits
    .space UVISOR_SRAM_LENGTH
