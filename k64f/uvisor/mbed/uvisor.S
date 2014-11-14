#include "../config/config.h"

.globl uvisor_init
.type  uvisor_init, %function

.section .text.uvisor, "x"
    .align 5
    .thumb
    .thumb_func
uvisor_init:
    .incbin "uvisor.bin"
    .align 5

.section .bss.uvisor,"awM",@nobits
    .align 5
    .space UVISOR_SRAM_SIZE
    .align 5
