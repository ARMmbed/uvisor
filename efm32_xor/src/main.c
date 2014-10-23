#include <iot-os.h>

static void xor_init(void)
{
    /* in this case, do nothing */
}

static uint32_t xor_enc(uint32_t a0)
{
    dprintf("arg: 0x%08X\n\r", a0);
    return a0 ^ 42;
}

static uint32_t xor_dec(uint32_t a0)
{
    dprintf("arg: 0x%08X\n\r", a0);
    return a0 ^ 42;
}

static void * const g_export_table[] =
{
    xor_init,        /* 0 - initialize    */
    xor_enc,        /* 1 - encrypt         */
    xor_dec,        /* 2 - decrypt         */
};

const ExportTable g_exports = {
    0,
    0,
    0,
    0,
    {0x00, 0x11, 0x22, 0x33,\
     0x44, 0x55, 0x66, 0x77,\
     0x88, 0x99, 0xAA, 0xBB,\
     0xCC, 0xDD, 0xEE, 0xFF},
    sizeof(g_export_table) / sizeof(g_export_table[0]),
    (uint32_t *) g_export_table
};
