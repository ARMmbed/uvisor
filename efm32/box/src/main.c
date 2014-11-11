#include <uvisor.h>
#include "xor_wrap.h"
#include "timer_wrap.h"
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
    /* WS2 set in uVisor - MSC access prohibited due to security reasons
     * FIXME: add system-register-level ACL's by adding SVC command */
    /* MSC->READCTRL = (MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK)|MSC_READCTRL_MODE_WS2; */
    CMU->CMD = CMU_CMD_HFCLKSEL_HFXO;

    /* Enable output */
    DEBUG_init();
}

void main_entry(void)
{
    dprintf("[unprivileged entry!]");

    /* initialize hardware */
    hardware_init();

    dprintf("hardware initialized, running main loop...\n");

    while(1)
    {
        /* xor encryption  */
        timer_start();
        timer_stop();
        dprintf("\nxor encryption...\n\r");
        dprintf("ret: 0x%08X\n\r", xor_enc(0xfeed));
        dprintf("...done.\n\n\r");

        DEBUG_TxByte('X');
    }
}

/* main application header table */
const HeaderTable g_hdr_tbl = {
    0xDEAD,                // magic value
    0,                // version
    0,                // length
    0,                // # of exported fn
    NULL,                // fn_table
    {0x44, 0x55, 0x66, 0x77,\
     0x44, 0x55, 0x66, 0x77,\
     0x44, 0x55, 0x66, 0x77,\
     0x44, 0x55, 0x66, 0x77},     // GUID
};

