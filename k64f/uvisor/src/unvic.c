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
TIsrUVector g_unvic_vector[ISR_VECTORS];

/* isr_default_handler to nvic_unpriv_default_handler */
void isr_default_handler(void) UVISOR_LINKTO(unvic_default_handler);

/* unprivileged default handler */
void unvic_default_handler(void)
{
    HALT_ERROR("spurious IRQ (IPSR=%i)", (uint8_t)__get_IPSR());
}

void unvic_isr_mux(void)
{
    return;
}

/* FIXME check if allowed (ACL) */
void unvic_set_isr(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    /* check if the IRQn is already registered */
    TIsrVector current_isr = ISR_GET(irqn);
    if(current_isr != (TIsrVector) &unvic_default_handler)
    {
        /* FIXME currently only ISR multiplexer is allowed as privileged ISR */
        if(current_isr != (TIsrVector) &unvic_isr_mux)
        {
            HALT_ERROR("Spurious ISR for IRQ %d: 0x%08X\n\r", irqn,
                                                              current_isr);
        }

        DPRINTF("IRQ %d already registered to box %d with vector 0x%08X\n\r",
                 irqn, g_unvic_vector[irqn].id, g_unvic_vector[irqn].hdlr);
        return;
    }

    DPRINTF("IRQ %d is now registered to box %d with vector 0x%08X\n\r",
             irqn, svc_cx_get_curr_id(), vector);

    /* save unprivileged handler */
    g_unvic_vector[irqn].hdlr = (TIsrVector) vector;
    g_unvic_vector[irqn].id   = svc_cx_get_curr_id();

    /* set privileged handler to unprivleged mux */
    ISR_SET(irqn, &unvic_isr_mux);

    /* set proper priority (SVC must always be higher) */
    /* FIXME currently only one level of priority is allowed */
    NVIC_SetPriority(irqn, 1);
}

uint32_t unvic_get_isr(uint32_t irqn)
{
    return 1;
}

void unvic_let_isr(uint32_t irqn)
{
    return;
}

/* FIXME check if allowed (ACL) */
void unvic_ena_irq(uint32_t irqn)
{
    /* FIXME currently only ISR multiplexer is allowed as privileged ISR */
    TIsrVector current_isr = ISR_GET(irqn);
    if((current_isr == (TIsrVector) &unvic_default_handler))
    {
        HALT_ERROR("No ISR set yet for IRQ %d\n\r", irqn);
    }
    if((current_isr != (TIsrVector) &unvic_isr_mux))
    {
        HALT_ERROR("Spurious ISR for IRQ %d: 0x%08X\n\r", irqn, current_isr);
    }

    /* check if the same box that registered the ISR is enabling it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR("Access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    DPRINTF("IRQ %d is now active and owned by box %d\n\r", irqn,
                                        g_unvic_vector[irqn].id);
    NVIC_EnableIRQ(irqn);
}

void unvic_dis_irq(uint32_t irqn)
{
    return;
}

void unvic_set_ena_isr(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    return;
}

void unvic_dis_let_isr(uint32_t isr)
{
    return;
}

void unvic_init(void)
{
    /* call original ISR initialization */
    isr_init();
}
