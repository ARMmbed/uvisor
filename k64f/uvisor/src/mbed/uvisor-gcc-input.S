#include "../config/config.h"

.globl uvisor_init
.type  uvisor_init, %function

.section .text.uvisor, "x"
    .align 5
    .thumb
    .thumb_func
uvisor_init:
    .incbin TARGET_BIN
    .long UVISOR_MAGIC
    .long 0 //MBED_VERSION
    .long __descr_tbl_start
    .long __descr_tbl_end
    .align 5

.section .bss.uvisor,"awM",@nobits
    .align 5
    .space UVISOR_SRAM_SIZE
    .align 5
