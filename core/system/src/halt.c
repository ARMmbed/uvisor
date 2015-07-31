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
