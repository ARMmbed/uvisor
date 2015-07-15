/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include "halt.h"

void halt_led(THaltError reason)
{
    uint32_t toggle;
    volatile uint32_t delay;

    /* enable clock for PORTB */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    /* set PORTB pin 22 to ALT1 function (GPIO) */
    PORTB->PCR[22] = PORT_PCR_MUX(1)|PORT_PCR_DSE_MASK;

    /* enable output for PORTB pin 21/22 */
    PTB->PDDR |= (1UL << 22);

    /* initially set PORTB pin 22 (LED active low) */
    PTB->PSOR = 1UL << 22;

    while(1)
    {
        for(toggle = 0; toggle < 2 * (uint32_t) reason; toggle++)
        {
            /* toggle PORTB pin 22 */
            PTB->PTOR = (1UL << 22);
            for(delay = 0; delay < HALT_INTRA_PATTERN_DELAY; delay++);
        }
        for(delay = 0; delay < HALT_INTER_PATTERN_DELAY; delay++);
    }
}
