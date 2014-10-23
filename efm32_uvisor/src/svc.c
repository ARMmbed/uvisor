#include <iot-os.h>
#include "svc.h"
#include "mpu.h"

#define CRYPTOBOX_API_MAX ((sizeof(g_svc_vector_table)/sizeof(g_svc_vector_table[0]))-1)

/* FIXME make globally available helper functions: thunk, halt_error */
extern void halt_error(const char *);
extern uint32_t thunk(uint32_t);

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

/* context switch from src to dst */
uint32_t switch_in(uint32_t dst_id, uint32_t a0)
{
    uint8_t stack_align = 0;
    uint8_t srd_mask;
    uint32_t *src_stack, src_box;
    uint32_t *dst_stack, dst_box, dst_fn;

    /* dst box number and function extraction from r0 */
    dst_box = dst_id >> 24;
    dst_fn = dst_id & 0x0FFF;

    /* src stack pointer and box number */
    mpu_check_permissions(src_stack = (uint32_t *)__get_PSP());
    src_box = *g_act_box >> 16;

    /* push src and dst box numbers on dedicated stack */
    if(!mpu_check_act_box_stack())
        return -1;
    *(--g_act_box) = (dst_box << 16) | (src_box);

    /* src stack pointer saved */
    g_box_stack[src_box] = src_stack;

    /* dst exception stack frame created */
    dst_stack = g_box_stack[dst_box] - EXC_SF_SIZE;

    /* check double-word stack alignment */
    if(((uint32_t) dst_stack & 0x7) != 0x0)
    {
        dst_stack--;
        stack_align = 1;
    }

    /* populate dst exception stack frame */
    memcpy((void *) dst_stack, (void *) src_stack, sizeof(uint32_t) * EXC_SF_SIZE);    /* copy stack         */
    dst_stack[5] = (uint32_t) thunk;                        /* lr             */
    dst_stack[6] = (uint32_t) BOX_TABLE(dst_box)->fn_table[dst_fn];            /* return address     */
    dst_stack[7] &= ~(1 << 9);                            /* clear xPSR[9]     */
    dst_stack[7] |= stack_align << 9;                        /* set custom xPSR[9]     */

    /* dst stack pointer saved and selected */
    g_box_stack[dst_box] = dst_stack;
    __set_PSP((uint32_t) dst_stack);

    /* mpu mask */
    srd_mask = (0xFF & ~(1 << 0)) & ~(1 << dst_box);

    /* enable stack region for dst */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    /* enable code region for dst */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    return a0;
}

/* context switch from dst to src */
uint32_t switch_out(uint32_t ret)
{
    uint8_t srd_mask;
    uint32_t *dst_stack;
    uint32_t src_box, dst_box;
    uint32_t tmp;

    /* pop src and dst box numbers from dedicated stack */
    if(!mpu_check_act_box_stack())
        return -1;
    tmp = *(g_act_box++);
    src_box = tmp & 0xFFFF;
    dst_box = tmp >> 16;

    /* dst stack pointer */
    mpu_check_permissions(dst_stack = (uint32_t *) __get_PSP());

    /* src stack pointer selected */
    __set_PSP((uint32_t) g_box_stack[src_box]);

    /* dst exception stack frame deleted */
    g_box_stack[dst_box] += (g_box_stack[dst_box][7] & 1 << 9) ? EXC_SF_SIZE + 1 : EXC_SF_SIZE;

    /* mpu mask */
    srd_mask = (0xFF & ~(1 << 0)) & ~(1 << src_box);

    /* disable stack region for dst */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    /* disable code region for dst */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    return ret;
}

uint32_t load_lib(Guid guid)
{
    int i, j;
    int flag = 0;

    /* look for GUID in all description tables */
    for(i = 1; i < MPU_REGION_SPLIT; i++)
    {
        for(j = 0; j < GUID_BYTE_SIZE; j++)
            if(BOX_TABLE(i)->guid[j] != guid[j])
                break;
        if(j == GUID_BYTE_SIZE)
        {
            /* check correct module relocation */
            mpu_check_fw_reloc(i);

            flag = 1;
            break;
        }
    }

    return flag ? i : 0;
}

__attribute__ ((section(".svc_vector")))
void* const g_svc_vector_table[] =
{
    cb_get_version,     /*  0 */
    cb_test_param0,     /*  1 */
    cb_test_param0,     /*  2 */
    cb_test_param1,     /*  3 */
    cb_test_param2,     /*  4 */
    cb_test_param3,     /*  5 */
    cb_test_param4,     /*  6 */
    switch_in,        /*  7 */        // context switch - start
    switch_out,        /*  8 */        // context_switch - end
    load_lib,        /*  9 */        // load library
};

/* FIXME change SVC interface to R0-based SVC number instead of using
 *       SVC-#imm
 * FIXME load registers from unprivileged stack before running
 *       SVC-handler to support tailchaining
 */
static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "push     {r4-r6, lr}\n"                 // save scratch registers
        "mrs     r5, PSP\n"
        "ldrt    r4, [r5, #24]\n"              // get unprivileged PC
        "sub     r4, #2\n"                      // seek back into SVC opcode
        "ldrbt     r4, [r4]\n"                  // read SVC# from opcode
        "svc_selection:\n"
        "ldr     r6, =g_svc_vector_table\n"     // get svc vector table offset
        "add     r6, r6, r4, LSL #2\n"          // svc table entry from SVC#
        "cmp     r4, %0\n"                      // verify maximum SVC#
        "bhi     abort\n"                      // abort if SVC# > max
        "ldr     r5, [r6]\n"                    // read corresponding SVC ptr
        "blx     r5\n"                          // call SVC handler
        "return:\n"
        "mrs     r5, PSP\n"                     // SP may have changed with context switch/
        "strt    r0, [r5]\n"                   // store R0 on return stack
        "pop     {r4-r6, pc}\n"                     // restore registers & return
        "abort:\n"                            // only needed if SVC# > max
        "eor     r0, r0\n"                      // reset r0 result
        "b     return\n"
        :: "i" (CRYPTOBOX_API_MAX)
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
