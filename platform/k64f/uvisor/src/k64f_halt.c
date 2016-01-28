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
