#include <iot-os.h>
#include "rsa.h"

void __attribute__((section(".rsa"))) thunk(void)
{
    __SVC0(8);
}

uint32_t __attribute__((section(".rsa"))) f_rsa(uint32_t a0)
{
    dprintf(" --- RSA (0x%x)---\n\r", a0);
    return 0xbeef;
}

uint32_t rsa(uint32_t a0)
{
    return __SVC3(7, a0, (uint32_t) f_rsa, (uint32_t) thunk);
}
