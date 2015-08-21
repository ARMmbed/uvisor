/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

void halt_user_error(THaltUserError reason)
{
    /* blink & die */
    halt_led(reason + __THALTERROR_MAX);
}
