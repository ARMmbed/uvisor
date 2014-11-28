#include <uvisor.h>
#include "svc.h"

#define SVC_HDLRS_NUM    ((sizeof(g_svc_vtor_tbl) / sizeof(g_svc_vtor_tbl[0])) - 1)

static void secure_switch(uint32_t addr)
{
    dprintf("switching for address: 0x%08X\n", addr);
}

static void svc_test()
{
    dprintf("svc_test\n");
}

__attribute__ ((section(".svc_vector"))) const void *g_svc_vtor_tbl[] = {
    secure_switch,        // 0
    svc_test,        // 1
};

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "tst    r12, #1\n"            // if ADDR_CMDn
        "itt    ne\n"
        "movne    r0, r12\n"            //   pass r12 as argument
        "bne    switch_in\n"            //   shortcut to switch_in

        "tst    lr, #4\n"            // otherwise regular SVC:
        "itee    ne\n"                //   select sp
        "mrsne    r0, PSP\n"
        "mrseq    r0, MSP\n"
        "beq    svc_num\n"

        "ldrt    r3, [r0, #12]\n"        // pass arguments to handler
        "ldrt    r2, [r0, #8]\n"            // (only unprivileged)
        "ldrt    r1, [r0, #4]\n"
        "ldrt    r0, [r0]\n"

        "svc_num:\n"
        "mov    r12, r12, LSR #1\n"        // SVC#
        "cmp    r12, %0\n"            // check if SVC# <= max
        "bhi    abort\n"            // abort if not
        "push    {r4}\n"
        "ldr    r4, =g_svc_vtor_tbl\n"
        "add    r12, r4, r12, LSL #2\n"        // offset in SVC vector table
        "ldr    r12, [r12]\n"
        "pop    {r4}\n"

        "bx    r12\n"                // execute SVC handler

        "abort:\n"
        "bx    lr\n"
        :: "i" (SVC_HDLRS_NUM)
    );
}

void svc_init(void)
{
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
