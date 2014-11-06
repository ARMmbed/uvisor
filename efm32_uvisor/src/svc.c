#include <iot-os.h>
#include "svc.h"
#include "mpu.h"

#define CRYPTOBOX_API_MAX ((sizeof(g_svc_vector_table)/sizeof(g_svc_vector_table[0]))-1)

/* FIXME make globally available helper functions: thunk, halt_error */
extern void halt_error(const char *);
extern uint32_t thunk(uint32_t);
extern void default_handler(void);
extern void redirect_handler(void);
extern volatile TIsrVector g_isr_vector_unpriv[MAX_ISR_VECTORS];

/* FIXME this only works without FPU */
/* context switch from src to dst */
void switch_in(void)
{
    uint8_t sp_align = 0;
    uint8_t srd_mask;
    uint32_t *src_sp, src_box;
    uint32_t dst_id, *dst_sp, dst_box, dst_fn;
    uint32_t tmp;

    /* src sp and box number */
    src_box = (*g_act_box & 0xFF00) >> 8;
    mpu_check_permissions(src_sp = (uint32_t *) __get_PSP());

    /* dst box number and function extraction from stacked r12 */
    dst_id = src_sp[4];
    dst_box = dst_id >> 20;
    dst_fn = (dst_id & 0xFFF00) >> 8;

    /* push src and dst box numbers on dedicated stack */
    if(!mpu_check_act_box_stack())
        halt_error("stack of active boxes under- or overflow");
    tmp = (*g_act_box & 0xFFFF0000) | (((dst_box << 8) | (src_box)) & 0x0000FFFF);
    *(--g_act_box) = tmp;

    /* src sp saved */
    g_boxes[src_box].sp = src_sp;

    /* dst exception sf created */
    dst_sp = g_boxes[dst_box].sp - EXC_SF_SIZE;

    /* check double-word stack alignment */
    if(((uint32_t) dst_sp & 0x7) != 0x0)
    {
        dst_sp--;
        sp_align = 1;
    }

    /* populate dst exception sf */
    memcpy((void *) dst_sp, (void *) src_sp, sizeof(uint32_t) * EXC_SF_SIZE);    // copy stack
    dst_sp[5] = (uint32_t) thunk;                            // lr
    dst_sp[6] = (uint32_t) BOX_TABLE(dst_box)->fn_table[dst_fn];            // return address
    dst_sp[7] &= ~(1 << 9);                                // clear xPSR[9]
    dst_sp[7] |= sp_align << 9;                            // set custom xPSR[9]

    /* dst sp saved and selected */
    g_boxes[dst_box].sp = dst_sp;
    __set_PSP((uint32_t) dst_sp);

    /* mpu mask */
    srd_mask = (0xFF & ~(1 << 0)) & ~(1 << dst_box);

    /* enable stack region for dst */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    /* enable code region for dst */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    return;
}

/* FIXME this only works without FPU */
/* context switch from dst to src */
void switch_out(void)
{
    uint8_t srd_mask;
    uint32_t *dst_sp;
    uint32_t src_box, dst_box;
    uint32_t tmp;

    /* pop src and dst box numbers from dedicated stack */
    if(!mpu_check_act_box_stack())
        halt_error("stack of active boxes under- or overflow");
    tmp = *(g_act_box++);
    src_box =  tmp & 0x00FF;
    dst_box = (tmp & 0xFF00) >> 8;

    /* dst sp */
    mpu_check_permissions(dst_sp = (uint32_t *) __get_PSP());

    /* src sp selected */
    __set_PSP((uint32_t) g_boxes[src_box].sp);

    /* result copied to src stacked r0 */
    g_boxes[src_box].sp[0] = dst_sp[0];

    /* dst exception sf deleted */
    g_boxes[dst_box].sp += (g_boxes[dst_box].sp[7] & 1 << 9) ? EXC_SF_SIZE + 1 : EXC_SF_SIZE;

    /* mpu mask */
    srd_mask = (0xFF & ~(1 << 0)) & ~(1 << src_box);

    /* disable stack region for dst */
    MPU->RNR  = 3;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    /* disable code region for dst */
    MPU->RNR  = 4;
    MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

    return;
}

/* given a GUID, find the number of the box hosting its corresponding
 * library */
void find_box_num(void)
{
    uint32_t *psp;
    Guid *guid;
    int i, j;
    int flag = 0;

    /* fetch GUID from stacked r0 */
    mpu_check_permissions(psp = (uint32_t *) __get_PSP());
    guid = (Guid *) psp[0];

    /* look for GUID in g_boxes */
    for(i = 1; i < MPU_REGION_SPLIT; i++)
    {
        for(j = 0; j < GUID_BYTE_SIZE; j++)
            if(g_boxes[i].guid[j] != (*guid)[j])
                break;
        if(j == GUID_BYTE_SIZE)
        {
            flag = 1;
            break;
        }
    }

    /* copy result on stacked r0 */
    psp[0] = flag ? i : 0;

    return;
}

/* de-privilege execution and switch context (if needed) to handle
 * an unprivileged interrupt handler */
void nonbasethrdena_in(uint32_t *msp)
{
    uint8_t sp_align = 0;
    uint8_t srd_mask;
    uint32_t *act_sp, act_box;
    uint32_t *dst_sp, dst_box;
    uint32_t tmp;

    /* act box number */
    mpu_check_permissions(act_sp = (uint32_t *) __get_PSP());
    act_box = (*g_act_box & 0xFF00) >> 8;

    /* use ISR handler to determine dst box */
    dst_box = mpu_which_box(msp[0]);

    if(dst_box == act_box)
    {
        /* if the interrupt is handled by the active box
         *  no MPU reconfiguration is needed */

        /* act and dst sp are the same */
        dst_sp = act_sp;
    }
    else
    {
        /* if the interrupt is handled by an idle box
         * MPU reconfiguration is needed */

        /* dst sp taken from global array */
        dst_sp = g_boxes[dst_box].sp;

        /* act sp saved for later restoration */
        g_boxes[act_box].sp = act_sp;

        /* MPU mask */
        srd_mask = (0xFF & ~(1 << 0)) & ~(1 << dst_box);

        /* enable stack region for dst */
        MPU->RNR  = 3;
        MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

        /* enable code region for dst */
        MPU->RNR  = 4;
        MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);
    }

    /* push src and dst box numbers on dedicated stack */
    if(!mpu_check_act_box_stack())
        halt_error("stack of active boxes under- or overflow");
    tmp = (*g_act_box & 0x0000FFFF) | (((dst_box << 24) | (act_box << 16)) & 0xFFFF0000);
    *(--g_act_box) = tmp;

    /* dst exception sf created */
    dst_sp -= EXC_SF_SIZE;

    /* check double-word stack alignment */
    if((uint32_t) dst_sp & 0x4)
    {
        dst_sp--;
        sp_align = 1;
    }

    /* populate dst exception stack frame */
    memcpy((void *) dst_sp, (void *) msp, sizeof(uint32_t) * EXC_SF_SIZE);    // copy privileged stack to dst
    dst_sp[7] &= ~0x3FF;                            // clear IRQn and bit 9 in xPSR
    dst_sp[7] |= sp_align << 9;                        // set custom xPSR[9]

    /* set NONBASETHRDENA in CCR */
    SCB->CCR |= 1;

    /* dst stack pointer selected */
    __set_PSP((uint32_t) dst_sp);

    /* FIXME this leverages the known number of stacked words
     *      which is compilation-dependent */
    uint32_t *current_msp = (uint32_t *) __get_MSP();
    current_msp[5] |= 0x1C;

    /* CONTROL[1:0] modified for unprivileged execution */
    __set_CONTROL(__get_CONTROL() | 3);

    return;
}

/* re-privilege execution and switch context (if needed) after
 * an unprivileged interrupt handler */
void nonbasethrdena_out(uint32_t *msp)
{
    uint8_t srd_mask;
    uint32_t *dst_sp, dst_box;
    uint32_t *act_sp, act_box;
    uint32_t tmp;

    /* pop src and dst box numbers from dedicated stack */
    if(!mpu_check_act_box_stack())
        halt_error("stack of active boxes under- or overflow");
    tmp = *(g_act_box++);
    act_box = (tmp & 0x00FF0000) >> 16;
    dst_box = (tmp & 0xFF000000) >> 24;

    /* dst sp */
    mpu_check_permissions(dst_sp = (uint32_t *) __get_PSP());

    /* copy return address from handler sf to privileged sf */
    msp[6] = dst_sp[6];

    if(act_box == dst_box)
    {
        /* if the interrupt was handled by the active box
         *  no MPU reconfiguration is needed */

        /* dst exception sf deleted */
        act_sp = dst_sp + ((dst_sp[7] & 1 << 9) ? EXC_SF_SIZE + 1 : EXC_SF_SIZE);
    }
    else
    {
        /* if the interrupt was handled by an idle box
         * MPU reconfiguration is needed */

        /* act sp is taken from global array */
        act_sp = g_boxes[act_box].sp;

        /* MPU mask */
        srd_mask = (0xFF & ~(1 << 0)) & ~(1 << act_box);

        /* enable stack region for dst */
        MPU->RNR  = 3;
        MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);

        /* enable code region for dst */
        MPU->RNR  = 4;
        MPU->RASR = (MPU->RASR & ~(MPU_RASR_SRD(0xFF))) | MPU_RASR_SRD(srd_mask);
    }

    /* act sp selected */
    __set_PSP((uint32_t) act_sp);

    /* set NONBASETHRDENA in CCR */
    SCB->CCR &= ~1;

    /* CONTROL[1:0] modified for unprivileged execution */
    __set_CONTROL(__get_CONTROL() & ~2);

    /* FIXME this leverages the known number of stacked words
     *      which is compilation-dependent */
    uint32_t *current_msp = (uint32_t *) __get_MSP();
    current_msp[5] |= 0x10;
    current_msp[5] &= ~0xC;

    return;
}

/* FIXME make sure the handler provided is inside the box that
 *      has asked for registration */
void register_isr(void)
{
    uint32_t *psp;
    uint32_t irqn;
    TIsrVector hdlr;

    /* fetch arguments from stacked r0 and r1 */
    mpu_check_permissions(psp = (uint32_t *) __get_PSP());
    irqn = psp[0];
    hdlr = (TIsrVector) psp[1];

    /* halt if (other) isr already present */
    if((g_isr_vector[irqn] != (TIsrVector) default_handler &&
        g_isr_vector[irqn] != (TIsrVector) redirect_handler))
        halt_error("another privileged ISR already in the requested position");
    /* else set isr and enable interrupt */
    else
    {
        if(!g_isr_vector_unpriv[irqn])
        {
            g_isr_vector_unpriv[irqn] = hdlr;
            ISR_SET(irqn - IRQn_OFFSET, redirect_handler);
            NVIC_EnableIRQ(irqn - IRQn_OFFSET);
            NVIC_SetPriority(TIMER1_IRQn, 0x1);
        }
        else
            if(g_isr_vector_unpriv[irqn] != hdlr)
                halt_error("another unprivileged ISR already in the requested position");
    }
}

__attribute__ ((section(".svc_vector")))
void* const g_svc_vector_table[] =
{
    switch_in,        /*  0 */        // context switch - start
    switch_out,        /*  1 */        // context_switch - end
    find_box_num,        /*  2 */        // load library
    nonbasethrdena_in,    /*  3 */        // de-privilege interrupt handler
    nonbasethrdena_out,    /*  4 */        // re-privilege after interrupt
    register_isr,        /*  5 */        // register unprivileged isr
};

/* FIXME load registers from unprivileged stack before running
 *       SVC-handler to support tailchaining
 */
/* SVC ISR multiplexer
 *     the correct hanlder is selected from a table based on
 *     the provided SVC# (passed through r12); by default
 *     the privileged sp (MSP) is saved in r0 for handlers
 *     that need the original exception sf */
static void __attribute__((naked)) __svc_irq(void)
{
    asm volatile(
        "mrs    r0, MSP\n"            // by default store privileged sp
        "and    r1, r12, #0xFF\n"        // SVC# is in LSByte of r12
        "cmp    r1, %0\n"            // check SVC# against max
        "bhi    abort\n"
        "ldr    r2, =g_svc_vector_table\n"    // SCV handler table
        "add    r2, r2, r1, LSL #2\n"        // SVC handler table offset
        "ldr    r2, [r2]\n"            // fetch SVC handler
        "bx    r2\n"                // execute SVC handler

        "abort:\n"
        "bx    lr\n"                // return with no handler executed
        :: "i" (CRYPTOBOX_API_MAX)
    );
}

void svc_init(void)
{
    /* register SVC call interface */
    ISR_SET(SVCall_IRQn, &__svc_irq);
}
