#include <iot-os.h>
#include "timer_wrap.h"

/* always called after reset handler */
void main_entry(void)
{
    dprintf("xor initialized!\n");

    /* do nothing */
}

static uint32_t xor_enc(uint32_t a0)
{
    /* start timer */
    timer_start();

    /* verify argument */
    dprintf("arg: 0x%08X\n\r", a0);

    /* stop timer */
    timer_stop();

    /* encrypt */
    return a0 ^ 42;
}

static void * const g_export_table[] =
{
    reset_handler,        // 0 - initialize
    xor_enc,        // 1 - encrypt
};

/* header table */
const HeaderTable g_hdr_tbl = {
    0xDEAD,
    0,
    0,
    sizeof(g_export_table) / sizeof(g_export_table[0]),
    (uint32_t *) g_export_table,
    {0x00, 0x11, 0x22, 0x33,\
     0x44, 0x55, 0x66, 0x77,\
     0x88, 0x99, 0xAA, 0xBB,\
     0xCC, 0xDD, 0xEE, 0xFF},
};
