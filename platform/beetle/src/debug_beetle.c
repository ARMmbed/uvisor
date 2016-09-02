/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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

typedef struct
{
    __IO uint32_t DATA;
    __IO uint32_t STATE;
    __IO uint32_t CTRL;
    union {
        __I uint32_t INTSTATUS;
        __O uint32_t INTCLEAR;
    };
    __IO uint32_t BAUDDIV;
} UART_Type;

#define UART1_BASE  0x40005000
#define UART1       ((UART_Type *) UART1_BASE)

static int beetle_uart1_initialized = 0;

/* Initialize the Uart 1 of Beetle SoC */
static void beetle_uart1_initialize(void)
{
    beetle_uart1_initialized = 1;
    UART1->CTRL = 0x3;
    UART1->BAUDDIV = 0x9C4;
}

/*
 * Check the status of the Beetle SoC Uart1
 * return 1 if serial is busy
 */
static int beetle_uart1_busy(void)
{
    return (UART1->STATE & 0x1);
}

/* Implementation of default_putc */
void default_putc(uint8_t data)
{
    if(beetle_uart1_initialized == 0)
        beetle_uart1_initialize();

    char c = (char) data;

    while(beetle_uart1_busy());
        UART1->DATA = c;
}
