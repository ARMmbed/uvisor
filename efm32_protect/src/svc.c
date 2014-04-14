#include <iot-os.h>
#include "svc.h"

/* issue SVC API aliases */
#define SVC_SERVICE(id, func_name, param...) void func_name(param) { SVC(id); }
#include "svc.h"

#define CRYPTOBOX_API_MAX ((sizeof(g_svc_vector_table)/sizeof(g_svc_vector_table[0]))-1)

uint32_t __attribute__ ((weak)) default_handler(void)
{
    return E_INVALID;
}

static uint32_t cb_get_version(uint32_t svcn)
{
    dprintf("svcn=0x%08X\r\n", svcn);
    return 0x44221100;
}

static uint32_t cb_test_param0(void)
{
    dprintf("cb_test_param0()\r\n");
    return 0x1;
}

static uint32_t cb_test_param1(uint32_t p1)
{
    dprintf("cb_test_param1(0x%08X)\r\n", p1);
    return 0x12;
}

static uint32_t cb_test_param2(uint32_t p1, uint32_t p2)
{
    dprintf("cb_test_param2(0x%08X,0x%08X)\r\n", p1, p2);
    return 0x0123;
}

static uint32_t cb_test_param3(uint32_t p1, uint32_t p2, uint32_t p3)
{
    dprintf("cb_test_param3(0x%08X,0x%08X,0x%08X)\r\n", p1, p2, p3);
    return 0x1234;
}

static uint32_t cb_test_param4(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    dprintf("cb_test_param4(0x%08X,0x%08X,0x%08X,0x%08X)\r\n", p1, p2, p3, p4);
    return 0x12345;
}

__attribute__ ((section(".svc_vector")))
void* const g_svc_vector_table[] =
{
    cb_get_version, /* 0 */
    cb_test_param0, /* 1 */
    cb_test_param0, /* 2 */
    cb_test_param1, /* 3 */
    cb_test_param2, /* 4 */
    cb_test_param3, /* 5 */
    cb_test_param4, /* 6 */
};

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "PUSH {r4-r6, lr}\n"            /* save scratch registers      */
        "MRS r5, PSP\n"                 /* store unprivilged SP to r5  */
        "LDR r6, =g_svc_vector_table\n" /* get svc vector table offset */
        "LDRT r4, [r5, #24]\n"          /* get unprivileged PC         */
        "SUB r4, #2\n"                  /* seek back into SVC opcode   */
        "LDRBT r4, [r4]\n"              /* read SVC# from opcode       */
        "ADD r4, r6, r4, LSL #2\n"      /* svc table entry from SVC#   */
        "LDR r4, [r4]\n"                /* read corresponding SVC ptr  */
        "BLX r4\n"                      /* call SVC handler            */
        "STRT r0, [r5]\n"               /* store R0 to return stack    */
        "POP {r4-r6, pc}\n"             /* resture registers & return  */
        ::"i"(cb_get_version),"i"(CRYPTOBOX_API_MAX)
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
