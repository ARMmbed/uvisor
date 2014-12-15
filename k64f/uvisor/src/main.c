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

#ifdef  NV_CONFIG_OFFSET
__attribute__ ((section(".nv_config")))
const NV_Type nv_config = {
    /* backdoor key */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* flash protection */
    0xFF, 0xFF, 0xFF, 0xFF,
    /* FSEC */
    0xFF,
    /* FOPT */
    0xFF,
    /* FEPROT */
    0xFF,
    /* FDPROT */
    0xFF
};
#endif/*NV_CONFIG_OFFSET*/

NOINLINE int uvisor_init(void)
{
    int res;

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

    /* init MPU */
    if((res = vmpu_init())!=0)
        return res;

    /* init SVC call interface */
    svc_init();

    DPRINTF("uvisor initialized\n");

    return 0;
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
}

void main_entry(void)
{
    /* initialize uvisor */
    if(!uvisor_init())
    {
        /* swap stack pointers*/
        __disable_irq();
        __set_PSP(__get_MSP());
        __set_MSP(((uint32_t)&__stack_end__)-STACK_GUARD_BAND);
        __enable_irq();

        /* apply box ACLs */
        scan_box_acls();

        /* switch to unprivileged mode.
         * this is possible as uvisor code is readable by unprivileged.
         * code and only the key-value database is protected from the.
         * unprivileged mode */
        __set_CONTROL(__get_CONTROL()|3);
        __ISB();
        __DSB();
    }
}
