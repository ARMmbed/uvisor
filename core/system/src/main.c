/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
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

TIsrVector g_isr_vector_prev;

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
        /* disable IRQ's to perform atomic swaps */
        __disable_irq();

        /* remember previous VTOR table */
        g_isr_vector_prev = (TIsrVector)SCB->VTOR;
        /* swap vector tables */
        SCB->VTOR = (uint32_t)&g_isr_vector;
        /* swap stack pointers*/
        __set_PSP(__get_MSP());
        __set_MSP((uint32_t)&__stack_top__);

        /* enable IRQ's after swapping */
        __enable_irq();

        /* finish initialization */
        uvisor_init_post();

        /* switch to unprivileged mode; this is possible as uvisor code is
         * readable by unprivileged code and only the key-value database is
         * protected from the unprivileged mode */
        __set_CONTROL(__get_CONTROL() | 3);
        __ISB();
        __DSB();
    }
}
