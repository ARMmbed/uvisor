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

NOINLINE int uvisor_init(void)
{
    int res;

    /* init MPU */
    if((res = vmpu_init())!=0)
        return res;

    /* init SVC call interface */
    svc_init();

    DPRINTF("uvisor initialized\n");

    return 0;
}

void main_entry(void)
{
    /* initialize uvisor */
    if(!uvisor_init())
    {
        /* swap stack pointers*/
        __disable_irq();
        __set_PSP(__get_MSP());
        __set_MSP((uint32_t)&__stack_end__);
        __enable_irq();

        /* switch to unprivileged mode.
         * this is possible as uvisor code is readable by unprivileged.
         * code and only the key-value database is protected from the.
         * unprivileged mode */
        __set_CONTROL(__get_CONTROL()|3);
        __ISB();
        __DSB();
    }
}
