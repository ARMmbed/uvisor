#include <iot-os.h>
#include "svc.h"

/* issue SVC API aliases */
#define SVC_SERVICE(id, func_name, param...) void func_name(param) { SVC(id); }
#include "svc.h"

uint32_t __attribute__ ((weak)) default_handler(void)
{
    return E_INVALID;
}

static uint32_t cb_get_version(uint32_t svcn)
{
    dprintf("svcn=0x%08X\r\n", svcn);
    return 0x44221100;
}

static uint32_t cb_reset(void)
{
    dprintf("reset\r\n");
    return 0;
}

const void* g_cryptobox_api[] =
{
    cb_get_version,
    cb_reset
};

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "PUSH {r4-r5, lr}\n"
        "MRS r5, PSP\n"
        "LDRT r4, [r5, #24]\n"
        "SUB r4, #2\n"
        "LDRBT r4,[r4]\n"
        "MOV r0, r4\n"
        "BL %0\n"
        "STRT r0, [r5]\n"
        "POP {r4-r5, pc}\n"
        ::"i"(cb_get_version):"r0"
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
