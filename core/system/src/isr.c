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
#include <isr.h>

/* actual vector table in RAM */
__attribute__ ((section(".isr_vector"), aligned(128)))
TIsrVector *g_isr_vector_prev, g_isr_vector[MAX_ISR_VECTORS];

void UVISOR_WEAK isr_default_handler(void)
{
    while(1);
}

void isr_init(void)
{
    /* vector table relocation */
    uint32_t *dst = (uint32_t *) &g_isr_vector;
    while(dst < ((uint32_t *) &g_isr_vector[MAX_ISR_VECTORS]))
        *dst++ = (uint32_t) &isr_default_handler;

    if(SCB->VTOR != (uint32_t)&g_isr_vector)
    {
        /* remember previous vector table */
        g_isr_vector_prev = (TIsrVector*)SCB->VTOR;
        /* udate vector table to new SRAM table */
        SCB->VTOR = (uint32_t) &g_isr_vector;
    }
}

#ifdef NOSYSTEM

void reset_handler(void)
{
    /* reset VTOR table pointer */
    g_isr_vector_prev = NULL;

    /* call main program */
    main_entry();
}

#else/*NOSYSTEM*/

void __attribute__ ((weak, noreturn)) default_handler(void)
{
    while(1);
}

void UVISOR_NORETURN reset_handler(void)
{
    uint32_t *dst;
    const uint32_t* src;

    /* initialize data RAM from flash*/
    src = &__data_start_src__;
    dst = &__data_start__;
    while(dst<&__data_end__)
        *dst++ = *src++;

    /* set bss to zero */
    dst = &__bss_start__;
    while(dst<&__bss_end__)
        *dst++ = 0;

    /* initialize interrupt service routines */
    isr_init();

    /* call main program */
    main_entry();

    while(1);
}

/* initial vector table - just needed for boot process */
__attribute__ ((section(".isr_vector_tmp")))
const TIsrVector g_isr_vector_tmp[] =
{
#ifdef  STACK_POINTER
    (TIsrVector)STACK_POINTER,           /* override Stack pointer if needed */
#else /*STACK_POINTER*/
    (TIsrVector)&__stack_end__,          /* initial Stack Pointer */
#endif/*STACK_POINTER*/
    reset_handler,                       /* reset Handler */
};

#endif/*NOSYSTEM*/
