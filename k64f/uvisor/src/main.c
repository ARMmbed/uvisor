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
#include "vmpu.h"
#include "svc.h"
#include "debug.h"

NOINLINE void uvisor_init(void)
{
    /* reset uvisor BSS */
    memset(
        &__bss_start__,
        0,
        VMPU_REGION_SIZE(&__bss_start__, &__bss_end__)
    );

    /* initialize data if needed */
    memcpy(
        &__data_start__,
        &__data_start_src__,
        VMPU_REGION_SIZE(&__data_start__, &__data_end__)
    );

    /* vector table relocation */
    uint32_t *dst = (uint32_t *) &g_isr_vector;
    while(dst < ((uint32_t *) &g_isr_vector[MAX_ISR_VECTORS]))
        *dst++ = (uint32_t) &default_handler;
    SCB->VTOR = (uint32_t) &g_isr_vector;

    /* init MPU */
    vmpu_init();

    /* init SVC call interface */
    svc_init();

    /* initialize debugging features */
    DEBUG_INIT();

    DPRINTF("uvisor initialized\n");
}

static void scan_box_acls(void)
{
    /* FIXME implement security context switch between main and
     *       secure box - hardcoded for now, later using ACLs from
     *       UVISOR_BOX_CONFIG */

    /* the linker script creates a list of all box ACLs configuration
     * pointers from __uvisor_cfgtbl_start to __uvisor_cfgtbl_end */

    AIPS0->PACRM &= ~(1 << 14); // MCG_C1_CLKS (PACRM[15:12])
    AIPS0->PACRN &= ~(1 << 22); // UART0       (PACRN[23:20])
    AIPS0->PACRJ &= ~(1 << 30); // SIM_CLKDIV1 (PACRJ[31:28])
    AIPS0->PACRJ &= ~(1 << 22); // PORTB mux   (PACRJ[23:20])
    AIPS0->PACRG &= ~(1 << 2);  // PIT module  (PACRJ[ 3: 0])
}

void main_entry(void)
{
    /* check uvisor mode */
    if(vmpu_check_mode())
        return;

    /* initialize uvisor */
    uvisor_init();

    /* swap stack pointers*/
    __disable_irq();
    __set_PSP(__get_MSP());
    __set_MSP(((uint32_t) &__stack_end__) - STACK_GUARD_BAND);
    __enable_irq();

    /* apply box ACLs */
    scan_box_acls();

    /* switch to unprivileged mode; this is possible as uvisor code is readable
     * by unprivileged code and only the key-value database is protected from
     * the unprivileged mode */
    __set_CONTROL(__get_CONTROL() | 3);
    __ISB();
    __DSB();
}
