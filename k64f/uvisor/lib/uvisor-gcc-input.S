#include "../config/config.h"

.globl uvisor_init
.globl uvisor_config
.type  uvisor_init, %function
.weak  __uvisor_mode

.section .text.uvisor, "x"
    .align 5
    .thumb
    .thumb_func

uvisor_init:
    /* uvisor - precompiled blob */
    .incbin TARGET_BIN

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

    /* uvisor default mode - user can override weak reference */
__uvisor_mode:
    .long 0

    .align 5

.section .bss.uvisor,"awM",@nobits
    .align 5
    .space UVISOR_SRAM_SIZE
    .align 5
