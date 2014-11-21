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

/* load required libraries */
#if    defined(APP_CLIENT) || defined(LIB_CLIENT)
void load_boxes(void)
{
    uint32_t *box_loader = &__box_init_start__;

    while(box_loader < &__box_init_end__)
        (*((BoxInitFunc)((uint32_t) *(box_loader++))))();
}
#endif/*defined(APP_CLIENT) || defined(LIB_CLIENT)*/

void reset_handler(void)
{
    uint32_t *dst;
    const uint32_t* src;

    /* initialize data RAM from flash*/
    src = &__data_start_src__;
    dst = &__data_start__;
    while(dst<&__data_end__)
        *dst++ = *src++;

#if defined(UVISOR)
    /* set VTOR to default handlers */
    dst = (uint32_t*)&g_isr_vector;
    while(dst<((uint32_t*)&g_isr_vector[MAX_ISR_VECTORS]))
        *dst++ = (uint32_t)&default_handler;
    SCB->VTOR = (uint32_t)&g_isr_vector;
#endif/*UVISOR && !NOSYSTEM*/

    /* set bss to zero */
    dst = &__bss_start__;
    while(dst<&__bss_end__)
        *dst++ = 0;

#if    defined(APP_CLIENT) || defined(LIB_CLIENT)
    load_boxes();
#endif/*defined(APP_CLIENT) || defined(LIB_CLIENT)*/

    main_entry();
#if     !defined(LIB_CLIENT) && !defined(NOSYSTEM)
    while(1);
#endif/*LIB_CLIENT*/
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
