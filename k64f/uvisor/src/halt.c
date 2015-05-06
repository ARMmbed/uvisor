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
#include "halt.h"

#define HALT_INTRA_PATTERN_DELAY 0x10000U
#define HALT_INTER_PATTERN_DELAY 0x100000U

void halt_led(THaltError reason)
{
    volatile uint32_t toggle, delay;

    /* enable clock for PORTB */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    /* set PORTB pin 22 to ALT1 function (GPIO) */
    PORTB->PCR[22] = PORT_PCR_MUX(1)|PORT_PCR_DSE_MASK;

    /* enable output for PORTB pin 21/22 */
    PTB->PDDR |= (1UL << 22);

    /* clear PORTB pin 22 */
    PTB->PCOR = 1UL << 22;

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

static void halt_putcp(void* p, char c)
{
    (void) p;
    default_putc(c);
}

static void halt_printf(const char *fmt, ...)
{
    /* print message using our halt_putcp function */
    va_list va;
    va_start(va, fmt);
    tfp_format(NULL, halt_putcp, fmt, va);
    va_end(va);
}

void halt_error(THaltError reason, const char *fmt, ...)
{
    halt_printf("HALT_ERROR: ");

    /* print actual error */
    va_list va;
    va_start(va, fmt);
    tfp_format(NULL, halt_putcp, fmt, va);
    va_end(va);

    /* final line feed */
    default_putc('\n');

    /* blink & die */
    halt_led(reason);
}

void halt_line(const char *file, uint32_t line, THaltError reason,
               const char *fmt, ...)
{
    halt_printf("HALT_ERROR(%s#%i): ", file, line);

    /* print actual error */
    va_list va;
    va_start(va, fmt);
    tfp_format(NULL, halt_putcp, fmt, va);
    va_end(va);

    /* final line feed */
    default_putc('\n');

    /* blink & die */
    halt_led(reason);
}
