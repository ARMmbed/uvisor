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
    return;
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
    return;
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
