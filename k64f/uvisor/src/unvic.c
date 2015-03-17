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
    /* check IRQn */
    if(irqn > IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* check if the IRQn is already registered */
    if(ISR_GET(irqn) != (TIsrVector) &unvic_default_handler)
    {
        HALT_ERROR("IRQ %d already registered to box %d with vector 0x%08X\n\r",
                   irqn, g_unvic_vector[irqn].id, g_unvic_vector[irqn].hdlr);
    }

    /* save unprivileged handler (unvic_default_handler if 0) */
    g_unvic_vector[irqn].hdlr = vector ? (TIsrVector) vector :
                                         &unvic_default_handler;
    g_unvic_vector[irqn].id   = svc_cx_get_curr_id();

    /* set privileged handler to unprivleged mux */
    ISR_SET(irqn, &unvic_isr_mux);

    /* set proper priority (SVC must always be higher) */
    /* FIXME currently only one level of priority is allowed */
    NVIC_SetPriority(irqn, 1);

    DPRINTF("IRQ %d is now registered to box %d with vector 0x%08X\n\r",
             irqn, svc_cx_get_curr_id(), vector);
}

uint32_t unvic_get_isr(uint32_t irqn)
{
    /* check IRQn */
    if(irqn > IRQ_VECTORS)
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
    if(irqn > IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

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
    if(irqn > IRQ_VECTORS)
    {
        HALT_ERROR("IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

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

void unvic_svc_cx_in(uint32_t *svc_sp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

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

        /* save the current state */
        svc_cx_push_state(src_id, src_sp, dst_id);

        /* switch boxes */
        vmpu_switch(dst_id);
    }
    else
    {
        dst_sp = src_sp;
    }

    /* create stack frame for de-privileging the interrupt */
    dst_sp = svc_cx_depriv_sf(msp, dst_sp);
    __set_PSP((uint32_t) dst_sp);

    /* enable non-base threading */
    SCB->CCR |= 1;

    /* de-privilege executionn */
    __set_CONTROL(__get_CONTROL() | 3);
}

void unvic_svc_cx_out(uint32_t *svc_sp)
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

    /* switch stack frames back */
    svc_cx_return_sf(src_sp, dst_sp);

    /* switch ACls */
    vmpu_switch(src_id);
    __set_PSP((uint32_t) src_sp);

    /* disable non-base threading */
    SCB->CCR &= ~1;

    /* re-privilege execution */
    __set_CONTROL(__get_CONTROL() & ~2);
}

void unvic_init(void)
{
    /* call original ISR initialization */
    isr_init();
}
