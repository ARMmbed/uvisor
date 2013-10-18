#include <iot-os.h>

/* initial vector table - just needed for boot process */
__attribute__ ((section(".isr_vector_tmp")))
const TIsrVector g_isr_vector_tmp[] =
{
    (TIsrVector)&__stack_end__,        /* Initial Stack Pointer */
    reset_handler,                    /* Reset Handler */
};

/* actual vector table in RAM */
__attribute__ ((section(".isr_vector")))
TIsrVector g_isr_vector[MAX_ISR_VECTORS];
