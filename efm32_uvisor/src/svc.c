#include <iot-os.h>
#include "svc.h"
#include "mpu.h"

#define CRYPTOBOX_API_MAX ((sizeof(g_svc_vector_table)/sizeof(g_svc_vector_table[0]))-1)

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

/* FIXME generalize to more than 1 function (more than 2 stacks) */
/* RSA dummy - context switch from box to rsa function */
void box_to_function(uint32_t a0, UnprivilegedFunctionEntry * new_f, UnprivilegedFunctionEntry * thunk)
{
    asm volatile(
        "PUSH    {r0}\n"
    );

    uint16_t stack_align = 0;

    /* enable stack region */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(0xF8);

    /* enable code region */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(0xF8);

    /* current sp */
    g_sp[1] = (uint32_t *) __get_PSP();

    /* function sf created */
    g_sp[2] -= EXC_SF_SIZE;

    /* check double-word stack alignment */
    if(((uint32_t) g_sp[2] & 0x7) != 0x0)
    {
        g_sp[2]--;
        stack_align = 1;
    }

    /* populate created sf */
    memcpy((void *) g_sp[2], (void *) g_sp[1], 4 * EXC_SF_SIZE);    /* copy stack */
    g_sp[2][5] = (uint32_t) thunk;                    /* lr */
    g_sp[2][6] = (uint32_t) new_f;                    /* return address */
    g_sp[2][7] &= ~(1 << 9);                    /* clear original xPSR */
    g_sp[2][7] |= stack_align << 9;                    /* set custom xPSR */

    /* function stack selected */
    __set_PSP((uint32_t) g_sp[2]);

    asm volatile(
        "POP    {r0}\n"
    );
}

/* RSA dummy - context switch from function to box */
void function_to_box(void)
{
    /* function sf deleted */
    g_sp[2] += (g_sp[2][7] & 1 << 9) ? EXC_SF_SIZE + 1 : EXC_SF_SIZE;

    /* box stack selected */
    __set_PSP((uint32_t) g_sp[1]);

    /* disable stack region */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(0xFC);

    /* disable code region */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(0xFC);
}

__attribute__ ((section(".svc_vector")))
void* const g_svc_vector_table[] =
{
    cb_get_version,     /* 0 */
    cb_test_param0,     /* 1 */
    cb_test_param0,     /* 2 */
    cb_test_param1,     /* 3 */
    cb_test_param2,     /* 4 */
    cb_test_param3,     /* 5 */
    cb_test_param4,     /* 6 */
    box_to_function,    /* 7 */        /* RSA dummy - context switch */
    function_to_box,    /* 8 */        /* RSA dummy - context switch */
};

/* FIXME change SVC interface to R0-based SVC number instead of using
 *       SVC-#imm
 * FIXME load registers from unprivileged stack before running
 *       SVC-handler to support tailchaining
 */

static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "  PUSH {r4-r6, lr}\n"            /* save scratch registers      */
        "  MRS r5, PSP\n"                 /* store unprivilged SP to r5  */
        "  LDR r6, =g_svc_vector_table\n" /* get svc vector table offset */
        "  LDRT r4, [r5, #24]\n"          /* get unprivileged PC         */
        "  SUB r4, #2\n"                  /* seek back into SVC opcode   */
        "  LDRBT r4, [r4]\n"              /* read SVC# from opcode       */
        "  ADD r6, r6, r4, LSL #2\n"      /* svc table entry from SVC#   */
        "  CMP r4, %0\n"                  /* verify maximum SVC#         */
        "  BHI abort\n"                   /* abort if SVC# > max         */
        "  LDR r4, [r6]\n"                /* read corresponding SVC ptr  */
        "  BLX r4\n"                      /* call SVC handler            */
        "return:\n"
        "  MRS r5, PSP\n"                 /* store unprivilged SP to r5  */ /* it may have changed */
        "  STRT r0, [r5]\n"               /* store R0 on return stack    */
        "  POP {r4-r6, pc}\n"             /* restore registers & return  */
        "abort:\n"                        /* only needed if SVC# > max   */
        "  EOR r0, r0\n"                  /* ..reset r0 result           */
        "  B return\n"
        ::"i"(CRYPTOBOX_API_MAX)
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
