/**************************************************************************//**
 * @file
 * @brief Debug printf on DEBUG_USART with a fixed baud rate of 115200, useful
 *        for debugging purposes. When Debug is not set this will not be
 *        compiled in. Note that __write is overridden in iar_write.c,
 *        which makes this work.
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
#include <iot-os.h>
#include "debug.h"

/* point USART_txByte to default_putc of printf */
void default_putc (uint8_t data) ALIAS(DEBUG_TxByte);

/**************************************************************************//**
 * @brief Initialize DEBUG_USART@115200 for use for debugging purposes.
 *****************************************************************************/
void DEBUG_init(void)
{
  /* Enable clock to DEBUG_USART */
  CMU->HFPERCLKEN0 |= DEBUG_USART_CLOCK;

  /* Set clock division */
  DEBUG_USART->CLKDIV = (int)((256*(XTAL_FREQUENCY/(16*DEBUG_USART_BAUD)-1))+0.5);

  /* Use default location 0: TX - pin C0, RX - pin C1 */
  DEBUG_USART->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN
                  | DEBUG_USART_LOCATION;

  CONFIG_DebugGpioSetup();

  /* Clear RX/TX buffers */
  DEBUG_USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
  /* Enable RX/TX */
  DEBUG_USART->CMD = USART_CMD_RXEN | USART_CMD_TXEN;
}

/**************************************************************************//**
 * @brief Transmit a single byte on usart1
 * @param data Character to transmit.
 *****************************************************************************/
void DEBUG_TxByte(uint8_t data)
{
  /* Check that transmit buffer is empty */
  while (!(DEBUG_USART->STATUS & USART_STATUS_TXBL)) ;

  DEBUG_USART->TXDATA = (uint32_t) data;
}
