/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include <isr.h>
#include "halt.h"
#include "unvic.h"
#include "svc.h"
#include "debug.h"

/* unprivileged vector table */
TIsrUVector g_unvic_vector[IRQ_VECTORS];

/* isr_default_handler to unvic_default_handler */
void isr_default_handler(void) UVISOR_LINKTO(unvic_default_handler);

/* unprivileged default handler */
void unvic_default_handler(void)
{
    HALT_ERROR("spurious IRQ (IPSR=%i)", (uint8_t)__get_IPSR());
}

/* ISR multiplexer to deprivilege execution of an interrupt routine */
void unvic_isr_mux(void)
{
    TIsrVector unvic_hdlr;
    uint32_t irqn;

    /* ISR number */
    irqn = (uint32_t)(uint8_t) __get_IPSR() - IRQn_OFFSET;
    if(!NVIC_GetActive(irqn))
    {
        HALT_ERROR("unvic_isr_mux() executed with no active IRQ\n\r");
    }

    /* ISR vector */
    if(!(unvic_hdlr = g_unvic_vector[irqn].hdlr))
    {
        HALT_ERROR("unprivileged handler for IRQ %d not found\n\r", irqn);
    }

    /* handle IRQ with an unprivileged handler */
    DPRINTF("IRQ %d being served with vector 0x%08X\n\r", irqn, unvic_hdlr);
    asm volatile(
        "svc  %[nonbasethrdena_in]\n"
        "blx  %[unvic_hdlr]\n"
        "svc  %[nonbasethrdena_out]\n"
        ::[nonbasethrdena_in]  "i" (SVC_MODE_PRIV_SVC_UNVIC_IN),
          [nonbasethrdena_out] "i" (SVC_MODE_UNPRIV_SVC_UNVIC_OUT),
          [unvic_hdlr]         "r" (unvic_hdlr)
    );
}

/* FIXME check if allowed (ACL) */
/* FIXME flag is currently not implemented */
void unvic_set_isr(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    uint32_t curr_id;

    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* get current ID to check ownership of the IRQn slot */
    curr_id = svc_cx_get_curr_id();

    /* check if the same box that registered the ISR is modifying it */
    if(ISR_GET(irqn) != (TIsrVector) &unvic_default_handler &&
       svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR("access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    /* save unprivileged handler (unvic_default_handler if 0) */
    g_unvic_vector[irqn].hdlr = vector ? (TIsrVector) vector :
                                         &unvic_default_handler;
    g_unvic_vector[irqn].id   = curr_id;

    /* set privileged handler to unprivleged mux */
    ISR_SET(irqn, &unvic_isr_mux);

    /* set default priority (SVC must always be higher) */
    NVIC_SetPriority(irqn, UNVIC_MIN_PRIORITY);

    DPRINTF("IRQ %d is now registered to box %d with vector 0x%08X\n\r",
             irqn, svc_cx_get_curr_id(), vector);
}

uint32_t unvic_get_isr(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* if the vector is unregistered return 0, otherwise the vector */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
        return 0;
    else
        return (uint32_t) g_unvic_vector[irqn].hdlr;
}

/* FIXME check if allowed (ACL) */
void unvic_enable_irq(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* check if ISR has been set for this IRQn slot */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
    {
        HALT_ERROR("no ISR set yet for IRQ %d\n\r", irqn);
    }

    /* check if the same box that registered the ISR is enabling it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR("access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    /* enable IRQ */
    DPRINTF("IRQ %d is now active\n\r", irqn);
    NVIC_EnableIRQ(irqn);
}

void unvic_disable_irq(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* check if ISR has been set for this IRQn slot */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
    {
        HALT_ERROR("no ISR set yet for IRQ %d\n\r", irqn);
    }

    /* check if the same box that registered the ISR is disabling it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR("access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    DPRINTF("IRQ %d is now disabled, but still owned by box %d\n\r", irqn,
                                                 g_unvic_vector[irqn].id);
    NVIC_DisableIRQ(irqn);
}

void unvic_set_priority(uint32_t irqn, uint32_t priority)
{
    /* unprivileged code cannot set priorities for system interrupts */
    if((int32_t) irqn < 0)
    {
        HALT_ERROR("access denied: IRQ %d is a system interrupt and is owned\
                    by uVisor\n\r", irqn);
    }
    else
    {
        /* check IRQn */
        if(irqn >= IRQ_VECTORS)
        {
            HALT_ERROR("IRQ %d out of range (%d to %d)\n\r",
                        irqn, 0, IRQ_VECTORS);
        }

        /* check priority */
        if(priority < UNVIC_MIN_PRIORITY)
        {
            HALT_ERROR("access denied: mimimum allowed priority is %d\n\r",
                        UNVIC_MIN_PRIORITY);
        }

        /* set priority for device specific interrupts */
        NVIC->IP[irqn] = ((priority << (8 - __NVIC_PRIO_BITS)) & 0xff);
    }

}

uint32_t unvic_get_priority(uint32_t irqn)
{
    /* unprivileged code only see a default 0-priority for system interrupts */
    if((int32_t) irqn < 0)
    {
        return 0;
    }
    else
    {
        /* check IRQn */
        if(irqn >= IRQ_VECTORS)
        {
            HALT_ERROR("IRQ %d out of range (%d to %d)\n\r",
                        irqn, 0, IRQ_VECTORS);
        }

        /* get priority for device specific interrupts  */
        return (uint32_t) (NVIC->IP[irqn] >> (8 - __NVIC_PRIO_BITS));
    }
}

/* naked wrapper function needed to preserve LR */
void UVISOR_NAKED unvic_svc_cx_in(uint32_t *svc_sp)
{
    asm volatile(
        "mov  r0, %[svc_sp]\n"                 /* 1st arg: svc_sp            */
        "push {lr}\n"                          /* save lr for later          */
        "bl   __unvic_svc_cx_in\n"             /* execute handler and return */
        "pop  {lr}\n"                          /* restore lr                 */
        "orr  lr, #0x1C\n"                     /* return with PSP, 8 words   */
        "bx   lr\n"
        :: [svc_sp] "r" (svc_sp)
    );
}

void __unvic_svc_cx_in(uint32_t *svc_sp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_sp_align;

    /* this handler is always executed from privileged code */
    uint32_t *msp = svc_sp;

    /* gather information from IRQn */
    uint32_t irqn = (uint32_t)(uint8_t) __get_IPSR();
    dst_id = g_unvic_vector[irqn].id;

    /* gather information from current state */
    src_id = svc_cx_get_curr_id();
    src_sp = svc_cx_validate_sf((uint32_t *) __get_PSP());

    /* a proper context switch is only needed if changing box */
    if(src_id != dst_id)
    {
        /* gather information from current state */
        dst_sp = svc_cx_get_curr_sp(dst_id);

        /* switch boxes */
        vmpu_switch(dst_id);
    }
    else
    {
        dst_sp = src_sp;
    }

    /* create unprivileged stack frame */
    dst_sp_align = ((uint32_t) dst_sp & 0x4) ? 1 : 0;
    dst_sp      -= (SVC_CX_EXC_SF_SIZE - dst_sp_align);

    /* copy stack frame for de-privileging the interrupt:
     *   - all registers are copied
     *   - pending IRQs are cleared in iPSR
     *   - store alignment for future unstacking */
    memcpy((void *) dst_sp, (void *) msp,
           sizeof(uint32_t) * 8);                  /* r0 - r3, r12, lr, xPSR */
    dst_sp[7] &= ~0x1FF;                           /* IPSR - clear IRQn      */
    dst_sp[7] |= dst_sp_align << 9;                /* xPSR - alignment       */

    /* save the current state */
    svc_cx_push_state(src_id, src_sp, dst_id, dst_sp);
    DEBUG_PRINT_SVC_CX_STATE();

    /* enable non-base threading */
    SCB->CCR |= 1;

    /* de-privilege executionn */
    __set_PSP((uint32_t) dst_sp);
    __set_CONTROL(__get_CONTROL() | 3);
}

/* naked wrapper function needed to preserve MSP and LR */
void UVISOR_NAKED unvic_svc_cx_out(uint32_t *svc_sp)
{
    asm volatile(
        "mov r0, %[svc_sp]\n"                  /* 1st arg; svc_sp            */
        "mrs r1, MSP\n"                        /* 2nd arg: msp               */
        "push {lr}\n"                          /* save lr for later          */
        "bl __unvic_svc_cx_out\n"              /* execute handler and return */
        "pop  {lr}\n"                          /* restore lr                 */
        "orr lr, #0x10\n"                      /* return with MSP, 8 words   */
        "bic lr, #0xC\n"
        "bx  lr\n"
        :: [svc_sp] "r" (svc_sp)
    );
}

void __unvic_svc_cx_out(uint32_t *svc_sp, uint32_t *msp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

    /* gather information from current state */
    dst_sp = svc_cx_validate_sf(svc_sp);
    dst_id = svc_cx_get_curr_id();

    /* gather information from previous state */
    svc_cx_pop_state(dst_id, dst_sp);
    src_id = svc_cx_get_src_id();
    src_sp = svc_cx_get_src_sp();
    DEBUG_PRINT_SVC_CX_STATE();

    /* copy return address of previous stack frame to the privileged one, which
     * was kept idle after interrupt de-privileging */
    msp[6] = dst_sp[6];

    if(src_id != dst_id)
    {
        /* switch ACls */
        vmpu_switch(src_id);
    }

    /* disable non-base threading */
    SCB->CCR &= ~1;

    /* re-privilege execution */
    __set_PSP((uint32_t) src_sp);
    __set_CONTROL(__get_CONTROL() & ~2);
}

void unvic_init(void)
{
    /* call original ISR initialization */
    isr_init();
}
