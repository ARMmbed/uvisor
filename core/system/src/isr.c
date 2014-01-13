#include <iot-os.h>
#include <isr.h>

/* actual vector table in RAM */
__attribute__ ((section(".isr_vector")))
TIsrVector g_isr_vector[MAX_ISR_VECTORS];

void __attribute__ ((weak, noreturn)) default_handler(void)
{
    while(1);
}

void __attribute__ ((noreturn)) reset_handler(void)
{
    uint32_t *dst;
    const uint32_t* src;

    /* initialize data RAM from flash*/
    src = &__data_start_src__;
    dst = &__data_start__;
    while(dst<&__data_end__)
        *dst++ = *src++;

    /* set VTOR to default handlers */
    dst = (uint32_t*)&g_isr_vector;
    while(dst<((uint32_t*)&g_isr_vector[MAX_ISR_VECTORS]))
        *dst++ = (uint32_t)&default_handler;
    SCB->VTOR = (uint32_t)&g_isr_vector;

    /* set bss to zero */
    dst = &__bss_start__;
    while(dst<&__bss_end__)
        *dst++ = 0;

    /* initialize system if needed */
    SystemInit();

    main_entry();
    while(1){};
}

/* initial vector table - just needed for boot process */
__attribute__ ((section(".isr_vector_tmp")))
const TIsrVector g_isr_vector_tmp[] =
{
#ifndef STACK_POINTER
    (TIsrVector)&__stack_end__,        /* Initial Stack Pointer */
#else /*STACK_POINTER*/
    (TIsrVector)STACK_POINTER,        /* Override Stack pointer if needed */
#endif/*STACK_POINTER*/
    reset_handler,                    /* Reset Handler */
};
