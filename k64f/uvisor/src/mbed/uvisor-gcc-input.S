#include "../config/config.h"

.globl uvisor_init
.type  uvisor_init, %function

.section .text.uvisor, "x"
    .align 5
    .thumb
    .thumb_func
uvisor_init:
    .incbin TARGET_BIN
    /* uvisor expects its configuration section right
       after itself in flash. If the configuration word is not found,
       uvisor will intentionally freeze to avoid accidentally
       unprotect systems */
    .long UVISOR_MAGIC
    .long 0 /* MBED_VERSION */
    .long 0 /* uvisor mode:
               0: disabled
               1: permissive
               2: enabled */
    .long __uvisor_cfgtbl_start
    .long __uvisor_cfgtbl_end
    .align 5

.section .bss.uvisor,"awM",@nobits
    .align 5
    .space UVISOR_SRAM_SIZE
    .align 5
