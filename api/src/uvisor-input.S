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
.globl __uvisor_ps
.type  uvisor_init, %function
.weak  __uvisor_mode
.weak  __uvisor_page_size
.globl __uvisor_priv_sys_irq_hooks

.section .uvisor.main, "x"
    .thumb
    .thumb_func

/* This is the unique uVisor entry point from the point of view of the host OS.
 * This symbol corresponds to:
 *      extern "C" void uvisor_init(void); */
uvisor_init:
    /* uVisor pre-compiled binary blob */
    .incbin UVISOR_BIN

/* The size of the uVisor export table is a uint32_t located at the end of the
 * uVisor binary. The uVisor export table is located at size number of bytes
 * before the end of the uVisor binary. Size includes the 4 bytes for size
 * itself. */
.set uvisor_export_table_size, uvisor_config - 4

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

    /* Main heap for box 0 */
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

    /* Privileged system IRQ hooks */
    .long __uvisor_priv_sys_irq_hooks

    /* Functions provided by uVisor Lib for use by uVisor in unprivileged mode
     * */
    .long __uvisor_lib_box_init

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

/* __uvisor_ps is written inside uvisor_init. It must not be
 * overwritten by libc init and therefore is placed in .uninitialized. */
.section .uninitialized
__uvisor_box_context:
__uvisor_ps:
    .long 0

/* Reserve space for the uVisor BSS section. This section is zeroed by uVisor at
 * boot time. */
.section .keep.uvisor.bss.main, "awM", @nobits
    .space UVISOR_SRAM_LENGTH_USED
