.global uvisor_init

#include "config/config.h"

.section .uvisor.text
.thumb
.align 5
uvisor_init:
.incbin "k64f_uvisor.bin"
.align 5

.section .uvisor.bss
.align 5
.comm uvisor_bss, RESERVED_SRAM
.align 5
