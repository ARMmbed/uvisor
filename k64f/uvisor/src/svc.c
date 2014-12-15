/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include "svc.h"
#include "vmpu.h"

/* max number of SVC handlers */
#define SVC_HDLRS_NUM (ARRAY_COUNT(g_svc_vtor_tbl) - 1)

static void secure_context_switch(uint32_t dst_addr)
{
}

void uvisor_bitband(uint32_t *addr, uint32_t val)
{
    *addr = val;
}

__attribute__ ((section(".svc_vector"))) const void *g_svc_vtor_tbl[] = {
    secure_context_switch,    // 0
    uvisor_bitband,        // 1
};

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "tst    r12, #1\n"            // if ADDR_CMDn
        "itt    ne\n"
        "movne    r0, r12\n"            //   pass r12 as argument
        "bne    secure_context_switch\n"    //   shortcut to switch_in

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
    /* register SVC handler */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
