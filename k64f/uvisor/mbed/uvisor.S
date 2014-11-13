#include "config/config.h"

.section .uvisor.text
.thumb
.thumb_func
.align 5
.globl uvisor_init
.type  uvisor_init, %function
uvisor_init:
.incbin "k64f_uvisor.bin"
.align 5

.section .uvisor.bss
.align 5
.comm uvisor_bss, RESERVED_SRAM
.align 5
