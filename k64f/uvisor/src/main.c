#include <uvisor.h>
#include "vmpu.h"
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

int uvisor_init(void)
{
    int t;
    volatile int i;

    /* verify config structure */
    if(__uvisor_config.magic!=UVISOR_MAGIC)
        while(1)
        {
            dprintf("config magic mismatch: &0x%08X=0x%08X - exptected 0x%08X\n",
                &__uvisor_config,
                __uvisor_config.magic,
                UVISOR_MAGIC);
            for(i = 0; i < 200000; i++);
        }

    /* bail if uvisor was disabled */
    if(!__uvisor_config.mode)
        return -1;

    dprintf("cfgtable: start=0x%08X end=0x%08X\n",
        __uvisor_config.cfgtbl_start,
        __uvisor_config.cfgtbl_end);

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

    return 0;
}

void main_entry(void)
{
    /* swap stack pointers*/
    __disable_irq();
    __set_PSP(__get_MSP());
    __set_MSP((uint32_t)&__stack_end__);
    __enable_irq();

    /* initialize uvisor */
    if(!uvisor_init())
    {
        /* switch to unprivileged mode.
         * this is possible as uvisor code is readable by unprivileged.
         * code and only the key-value database is protected from the.
         * unprivileged mode */
        __set_CONTROL(__get_CONTROL()|3);
        __ISB();
        __DSB();
    }
}
