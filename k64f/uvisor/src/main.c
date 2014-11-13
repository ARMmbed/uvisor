#include <uvisor.h>

void main_entry(void)
{
    int t;
    volatile int i;

    register uint32_t __lr asm("lr");
    uint32_t lr = __lr;

    /* reset previous channel settings */
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM->TER = 0x0;
    /* wait for debugger to connect */
    while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & (1<<CHANNEL_DEBUG))));

    t = 0;
    while(t < 10)
    {
        dprintf("Hello World %i!\n", t++);

        for(i = 0; i < 2000000; i++);
    }

    asm volatile(
        "blx    %0\n"
        :: "r" (lr)
    );

    /* should never happen */
    while(1)
        __WFI();
}
