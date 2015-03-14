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

/* isr_default_handler to nvic_unpriv_default_handler */
void isr_default_handler(void) UVISOR_LINKTO(nvic_unpriv_default_handler);

/* unprivileged default handler */
void nvic_unpriv_default_handler(void)
{
    HALT_ERROR("spurious IRQ (IPSR=%i)", (uint8_t)__get_IPSR());
}

void unvic_init(void)
{
    /* call original ISR initialization */
    isr_init();
}
