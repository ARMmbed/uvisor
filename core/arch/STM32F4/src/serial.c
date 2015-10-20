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

#include "serial.h"
#include "serial_api.h"


static serial_t stdio_uart;
static int is_serial_init = 0;

void uvisor_serial_init(void)
{
    serial_init(&stdio_uart, STDIO_UART_TX, STDIO_UART_RX);
    is_serial_init = 1;
}

int uvisor_serial_getc(void)
{
    if (!is_serial_init)
        return 0;

    return serial_getc(&stdio_uart);
}

void uvisor_serial_putc(int c)
{
    if (!is_serial_init)
        return;

    serial_putc(&stdio_uart, c);
}
