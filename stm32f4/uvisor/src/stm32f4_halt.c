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
