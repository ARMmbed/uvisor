#include <iot-os.h>
#include "debug.h"

void main(void)
{
    uint32_t i;

    /* Enable clocks for peripherals. */
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 = CMU_HFPERCLKEN0_GPIO;

    /* Enable output */
    DEBUG_init();

    i=0;
    while(true)
        dprintf("Hello World %06d\r\n",i++);
}
