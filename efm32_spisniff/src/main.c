#include <iot-os.h>
#include "debug.h"

static inline void hardware_init(void)
{
    /* Enable clocks for peripherals */
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 = CMU_HFPERCLKEN0_GPIO;

    /* Set calibration for 28MHz band crystal */
    CMU->HFRCOCTRL = CMU_HFRCOCTRL_BAND_28MHZ |
        ((DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
        >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT);
    while(!(CMU->STATUS & CMU_STATUS_HFRCORDY));

    /* Enable output */
    DEBUG_init();
}

void main(void)
{
    uint32_t i;

    /* initialize hardware */
    hardware_init();

    i=0;
    while(true)
        dprintf("Hello World 12345 %06d\r\n",i++);
}
