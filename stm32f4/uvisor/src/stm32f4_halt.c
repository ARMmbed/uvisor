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

#define HALT_LED_PIN 14

void halt_led(THaltError reason)
{
    uint32_t toggle;
    volatile uint32_t delay;

    int flag;

    /* enable clock for port C */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;

    /* enable output for pin */
    GPIOG->MODER &= ~(0x3 << (HALT_LED_PIN * 2));
    GPIOG->MODER |=   0x1 << (HALT_LED_PIN * 2) ;

    /* set (low) speed for pin */
    GPIOG->OSPEEDR &= ~(0x3 << (HALT_LED_PIN * 2));

    /* set pin as neither pull up nor pull down */
    GPIOG->PUPDR &= ~(0x3 << (HALT_LED_PIN * 2));

    flag = 0;
    while(1)
    {
        for(toggle = 0; toggle < 2 * (uint32_t) reason; toggle++)
        {
            /* toggle PORTG LED pin */
            GPIOG->BSRR |= 0x1 << (HALT_LED_PIN + (flag ? 16 : 0));
            flag = !flag;
            for(delay = 0; delay < HALT_INTRA_PATTERN_DELAY; delay++);
        }
        for(delay = 0; delay < HALT_INTER_PATTERN_DELAY; delay++);
    }
}
