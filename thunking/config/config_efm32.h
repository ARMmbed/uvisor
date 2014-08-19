/**************************************************************************//**
 * @file
 * @brief Bootloader Configuration.
 *    This file defines how the bootloader is set up.
 * @author Energy Micro AS
 * @version 1.65
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2013 Energy Micro AS, http://www.energymicro.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. The source and compiled code may only be used on Energy Micro "EFM32"
 *    microcontrollers and "EFR4" radios.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Energy Micro AS has no
 * obligation to support this Software. Energy Micro AS is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Energy Micro AS will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 *****************************************************************************/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/** Frequency of the LF clock */
#define LFRCO_FREQ           (32768)

/** Number of seconds before autobaud times out and restarts the bootloader */
#define AUTOBAUD_TIMEOUT     30

/** Number of milliseconds between each consecutive polling of the SWD pins */
#define PIN_LOOP_INTERVAL    250

/** The size of the bootloader flash image */
#define BOOTLOADER_SIZE      4096

/** USART used for communication. */
#define BOOTLOADER_USART           USART0
#define BOOTLOADER_USART_CLOCK     CMU_HFPERCLKEN0_USART0
#define BOOTLOADER_USART_LOCATION  USART_ROUTE_LOCATION_LOC0

/** TIMER1 is used for autobaud. The channel and location must match the
 * RX line of BOOTLOADER_USART for this to work properly. */
#define AUTOBAUD_TIMER             TIMER1
#define AUTOBAUD_TIMER_CHANNEL     1
#define AUTOBAUD_TIMER_LOCATION    TIMER_ROUTE_LOCATION_LOC1
#define AUTOBAUD_TIMER_IRQn        TIMER1_IRQn
#define AUTOBAUD_TIMER_CLOCK       CMU_HFPERCLKEN0_TIMER1

/** USART used for debugging. */
#define DEBUG_USART                UART1
#define DEBUG_USART_CLOCK          CMU_HFPERCLKEN0_UART1
#define DEBUG_USART_LOCATION       UART_ROUTE_LOCATION_LOC2
#define DEBUG_USART_BAUD           115200UL

/** LEDs used for debugging */
#define DEBUG_PORT                 4
#define DEBUG_LED0                 (1<<2)
#define DEBUG_LED1                 (1<<3)

/** This function sets up the GPIO setting for the debug output. */
static __INLINE void CONFIG_DebugGpioSetup(void)
{
  /* Avoid false start by setting output as high */
  GPIO->P[1].DOUT = (1 << 9);
  GPIO->P[1].MODEH = GPIO_P_MODEH_MODE9_PUSHPULL | GPIO_P_MODEH_MODE10_INPUT;

  GPIO->P[DEBUG_PORT].DOUT = (1 << 10) | DEBUG_LED0 | DEBUG_LED1;

  /* configure Debug LED's */
  GPIO->P[DEBUG_PORT].MODEL =
    GPIO_P_MODEL_MODE2_PUSHPULL|
    GPIO_P_MODEL_MODE3_PUSHPULL;
}

#endif
