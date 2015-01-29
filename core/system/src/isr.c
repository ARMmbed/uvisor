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

#ifdef  UVISOR
/* actual vector table in RAM */
__attribute__ ((section(".isr_vector"), aligned(128)))
TIsrVector g_isr_vector[MAX_ISR_VECTORS];

void __attribute__ ((weak, noreturn)) default_handler(void)
{
    while(1);
}
#endif/*UVISOR*/

void reset_handler(void)
{
    uint32_t *dst;
    const uint32_t* src;

    /* initialize data RAM from flash*/
    src = &__data_start_src__;
    dst = &__data_start__;
    while(dst<&__data_end__)
        *dst++ = *src++;

#ifdef    UVISOR
    /* set VTOR to default handlers */
    dst = (uint32_t*)&g_isr_vector;
    while(dst<((uint32_t*)&g_isr_vector[MAX_ISR_VECTORS]))
        *dst++ = (uint32_t)&default_handler;
    SCB->VTOR = (uint32_t)&g_isr_vector;
#endif/*UVISOR*/

    /* set bss to zero */
    dst = &__bss_start__;
    while(dst<&__bss_end__)
        *dst++ = 0;

    main_entry();
#if !defined(LIB_CLIENT) && !defined(NOSYSTEM)
    while(1);
#endif/*!defined(LIB_CLIENT) && !defined(NOSYSTEM)*/
}

#ifndef NOSYSTEM
/* initial vector table - just needed for boot process */
__attribute__ ((section(".isr_vector_tmp")))
const TIsrVector g_isr_vector_tmp[] =
{
#if      defined(APP_CLIENT) || defined(LIB_CLIENT)
    reset_handler,
#else /*defined(APP_CLIENT) || defined(LIB_CLIENT)*/
#ifdef  STACK_POINTER
    (TIsrVector)STACK_POINTER,        /* override Stack pointer if needed */
#else /*STACK_POINTER*/
    (TIsrVector)&__stack_end__,        /* initial Stack Pointer */
#endif/*STACK_POINTER*/
    reset_handler,                /* reset Handler */
#endif/*defined(APP_CLIENT) || defined(LIB_CLIENT)*/
};
#endif/*NOSYSTEM*/
