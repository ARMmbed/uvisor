#include <uvisor.h>

/* xor exported functions */
typedef enum {init = 0, enc} XorFunction;

/* xor GUID */
static const Guid xor_guid = {
    0x00, 0x11, 0x22, 0x33,
    0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB,
    0xCC, 0xDD, 0xEE, 0xFF
};

/* xor box number */
static uint32_t g_xor_box;

/* xor library loader */
static void xor_init(void)
{
    if(!(g_xor_box = get_box_num(xor_guid)))
        dprintf("Error. Box not found\n\r");
    return;
}

/* xor library loader (ptr) */
uint32_t __attribute__((section(".box_init"))) g_xor_init = (uint32_t) xor_init;

/* xor functions - encrypt */
uint32_t xor_enc(uint32_t a0)
{
    return context_switch_in(enc, g_xor_box);
}
