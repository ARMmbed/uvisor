#include <uvisor.h>
#include "vmpu.h"
#include "test_box.h"
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

void uvisor_init(void)
{
    int t;
    volatile int i;

    /* reset previous channel settings */
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM->TER = 0x0;
    /* wait for debugger to connect */
    while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & (1<<CHANNEL_DEBUG))));

    /* init MPU */
    vmpu_init();

    /* init SVC */
    svc_init();

    t = 0;
    while(t < 10)
    {
        dprintf("Hello World %i!\n", t++);
        for(i = 0; i < 200000; i++);
    }
}

void main_entry(void)
{
    /* swap stack pointers*/
    __disable_irq();
    __set_PSP(__get_MSP());
    __set_MSP((uint32_t)&__stack_end__);
    __enable_irq();

    /* initialize uvisor */
    uvisor_init();

#ifndef NOSYSTEM
    /* set up test box */
    vmpu_box_add(&test_box);
#endif/*NOSYSTEM*/

    /* switch to unprivileged mode.
     * this is possible as uvisor code is readable by unprivileged.
     * code and only the key-value database is protected from the.
     * unprivileged mode */
    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();
}
