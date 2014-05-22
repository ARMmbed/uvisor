#include <iot-os.h>
#include "debug.h"

static inline void hardware_init(void)
{
    /* Enable clocks for peripherals */
    CMU->CTRL |= (CMU_CTRL_HFLE | CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ);
    CMU->OSCENCMD =  CMU_OSCENCMD_HFXOEN;
    CMU->HFCORECLKDIV = CMU_HFCORECLKDIV_HFCORECLKLEDIV;
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    /* Set calibration for 48MHz crystal */
    CMU->HFRCOCTRL = CMU_HFRCOCTRL_BAND_28MHZ |
        ((DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
        >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT);
    while(!(CMU->STATUS & CMU_STATUS_HFRCORDY));
    MSC->READCTRL = (MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK)|MSC_READCTRL_MODE_WS2;
    CMU->CMD = CMU_CMD_HFCLKSEL_HFXO;

    /* Enable output */
    DEBUG_init();
}

void main_entry(void)
{
    dprintf("[unprivileged entry!]");

    /* SVC test calls */
    __SVC0(1);
    __SVC0(2);
    __SVC1(3,0x11111111);
    __SVC2(4,0x11111111,0x22222222);
    __SVC3(5,0x11111111,0x22222222,0x33333333);
    __SVC4(6,0x11111111,0x22222222,0x33333333,0x44444444);

    /* initialize hardware */
    hardware_init();

    while(1)
    {
        dprintf("Hello World\n");
    }
}
