#include <uvisor.h>

void main_entry(void)
{
    int t;
    volatile int i;

    /* reset previous channel settings */
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM->TER = 0x0;
    /* wait for debugger to connect */
    while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & (1<<CHANNEL_DEBUG))));

    t=0;
    while(1)
    {
        dprintf("Hello World %i!", t++);

        for(i=0; i<20000; i++);
    }

    /* should never happen */
    while(1)
        __WFI();
}
