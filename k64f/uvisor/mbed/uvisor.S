.global uvisor_init

#include "../config/config.h"

.section .uvisor.text

.align 5
uvisor_init:
.incbin "../k64f_uvisor.bin"
.align 5


.section .uvisor.bss

.align 5
.skip  RESERVED_SRAM
.align 5
