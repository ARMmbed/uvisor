#include <iot-os.h>
#include "svc.h"

/* issue SVC API aliases */
#define SVC_SERVICE(id, func_name, param...) void func_name(param) { SVC(id); }
#include "svc.h"

#define MAX_SVC_CALLS 4

/*
typedef uint32_t (*TSVC)(void);

const g_svc_call_table[MAX_SVC_CALLS] = {
}
*/

static void svc_irq(uint32_t svcn)
{
    dprintf("svcn=0x%08X\r\n", svcn);
}

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "PUSH {r4-r5, lr}\n"
        "MRS r4, PSP\n"
        "LDRT r4, [r4, #24]\n"
        "SUB r4, #2\n"
        "LDRBT r4,[r4]\n"
        "MOV r0, r4\n"
        "BL %0\n"
        "POP {r4-r5, pc}\n"
        ::"i"(svc_irq):"r0"
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
