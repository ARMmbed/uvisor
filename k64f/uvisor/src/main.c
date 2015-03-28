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
#include "unvic.h"
#include "debug.h"

UVISOR_NOINLINE void uvisor_init_pre(void)
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

    /* initialize debugging features */
    DEBUG_INIT();
}

UVISOR_NOINLINE void uvisor_init_post(void)
{
        /* vector table initialization */
        unvic_init();

        /* init MPU */
        vmpu_init_post();

        /* init SVC call interface */
        svc_init();

        DPRINTF("uvisor initialized\n");
}

void main_entry(void)
{
    /* initialize uvisor */
    uvisor_init_pre();

    /* run basic sanity checks */
    if(vmpu_init_pre() == 0)
    {
        /* swap stack pointers*/
        __disable_irq();
        __set_PSP(__get_MSP());
        __set_MSP(((uint32_t) &__stack_end__) - STACK_GUARD_BAND);
        __enable_irq();

        /* finish initialization */
        uvisor_init_post();

        /* switch to unprivileged mode; this is possible as uvisor code is readable
         * by unprivileged code and only the key-value database is protected from
         * the unprivileged mode */
        __set_CONTROL(__get_CONTROL() | 3);
        __ISB();
        __DSB();
    }
}
