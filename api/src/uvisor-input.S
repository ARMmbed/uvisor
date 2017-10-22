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
.globl uvisor_api
.globl uvisor_config
.globl __uvisor_ps
.weak  __uvisor_mode
.weak  __uvisor_page_size
.globl __uvisor_priv_sys_hooks
.weak  __uvisor_debug_driver

.section .uvisor.main, "x"

/* This is the uVisor API function table from the point of view of the host OS.
 * This symbol corresponds to:
 *      extern "C" UvisorApi uvisor_api; */
uvisor_api:
    /* uVisor pre-compiled binary blob */
    .incbin UVISOR_BIN

uvisor_config:
    /* uVisor expects its configuration section right after itself in flash. If
     * the configuration magic is not found, uVisor will intentionally freeze to
     * avoid accidentally unprotect systems. */
    .long UVISOR_MAGIC

    /* Host OS version */
    .long 0

    /* uVisor mode of operation */
    .long __uvisor_mode

    /* Protected BSS section */
    .long __uvisor_bss_start
    .long __uvisor_bss_end

    /* uVisor own BSS section */
    .long __uvisor_bss_main_start
    .long __uvisor_bss_main_end

    /* Seecure boxes BSS section */
    .long __uvisor_bss_boxes_start
    .long __uvisor_bss_boxes_end

    /* uVisor own code and data */
    .long __uvisor_main_start
    .long __uvisor_main_end

    /* Protected flash memory region */
    .long __uvisor_secure_start
    .long __uvisor_secure_end

    /* Secure boxes configuration tables */
    .long __uvisor_cfgtbl_start
    .long __uvisor_cfgtbl_end

    /* Pointers to the secure boxes configuration tables */
    .long __uvisor_cfgtbl_ptr_start
    .long __uvisor_cfgtbl_ptr_end

    /* Pointers to the secure boxes register gateways */
    .long __uvisor_register_gateway_ptr_start
    .long __uvisor_register_gateway_ptr_end

    /* Address of __uvisor_box_context */
    .long __uvisor_box_context

    /* Public heap (used by the public box) */
    .long __uvisor_heap_start
    .long __uvisor_heap_end

    /* Page allocator region */
    .long __uvisor_page_start
    .long __uvisor_page_end

    /* Page size for the page allocator */
    .long __uvisor_page_size

    /* Physical memory boundaries */
    .long __uvisor_flash_start;
    .long __uvisor_flash_end;
    .long __uvisor_sram_start;
    .long __uvisor_sram_end;

    /* Physical memory boundaries of the public SRAM.
     * This is usually the same as the other SRAM symbols above, but it's
     * different when uVisor uses a TCM as its own SRAM. */
    .long __uvisor_public_sram_start;
    .long __uvisor_public_sram_end;

    /* newlib impure pointer */
    .long _impure_ptr

    /* Privileged system hooks */
    .long __uvisor_priv_sys_hooks

    /* Functions provided by uVisor Lib for use by uVisor in unprivileged mode
     * */
    .long __uvisor_lib_hooks

    /* Debug driver pointer */
    .long __uvisor_debug_driver

    /* Stack limit for public box MSPLIM_NS, accessed by public_box_stack_limit */
    .long __StackLimit

/* uVisor mode of operation
 * Modes available: UVISOR_ENABLED, UVISOR_DISABLED, UVISOR_PERMISSIVE. */
__uvisor_mode:
    /* uVisor default mode: Disabled.
     * The user can override this weak reference. */
    .long 0

__uvisor_page_size:
    /* uVisor default page size: 16kB.
     * The user can override this weak reference. */
    .long 16384

__uvisor_debug_driver:
    /* No debug driver by default.
     * Populated per request by the application */
    .long 0
    .long 0
    .long 0
    .long 0

/* __uvisor_ps is written inside uvisor_init. It must not be
 * overwritten by libc init and therefore is placed in .uninitialized. */
.section .uninitialized
.balign 4
__uvisor_box_context:
__uvisor_ps:
    .long 0

/* Reserve space for the uVisor BSS section. This section is zeroed by uVisor at
 * boot time. */
.section .keep.uvisor.bss.main, "awM", @nobits
    .space UVISOR_SRAM_LENGTH_USED
