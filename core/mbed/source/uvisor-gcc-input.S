/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include "../config/config.h"

.globl uvisor_init
.globl uvisor_config
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

    /* uvisor default mode - user can override weak reference */
__uvisor_mode:
    .long 0

    .align 5

.section .uvisor.bss.main,"awM",@nobits
__uvisor_reserved_start:
    .space USE_SRAM_SIZE
__uvisor_reserved_end:
