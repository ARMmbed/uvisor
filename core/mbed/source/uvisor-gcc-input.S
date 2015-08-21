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
#include "../config/config.h"

.globl uvisor_init
.globl uvisor_config
.globl uvisor_ctx
.type  uvisor_init, %function
.weak  __uvisor_mode

.section .uvisor.main, "x"
    .thumb
    .thumb_func

uvisor_init:
    /* uvisor - precompiled blob */
    .align 5
    .incbin RELEASE_BIN

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

    /* start and end address of box list */
    .long __uvisor_cfgtbl_start
    .long __uvisor_cfgtbl_end

    /* flash origin of initialized secret data */
    .long __uvisor_data_src

    /* initialized secret data section */
    .long __uvisor_data_start
    .long __uvisor_data_end

    /* uninitialized secret data section */
    .long __uvisor_bss_start
    .long __uvisor_bss_end

    /* start and end of flash protection - remaining flash size is
       available for configuration storage */
    .long __uvisor_secure_start
    .long __uvisor_secure_end

    /* start and end of uvisor stack */
    .long __uvisor_reserved_start
    .long __uvisor_reserved_end

    /* pointer to __uvisor_box_context */
    .long __uvisor_box_context

    /* uvisor default mode - user can override weak reference */
__uvisor_mode:
    .long 0

    .align 5

.section .bss
__uvisor_box_context:
uvisor_ctx:
    .long 0

.section .uvisor.bss.main,"awM",@nobits
__uvisor_reserved_start:
    .space USE_SRAM_SIZE
__uvisor_reserved_end:
