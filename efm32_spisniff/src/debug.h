/**************************************************************************//**
 * @file
 * @brief Debug printf on USART1 with a fixed baud rate of 115200, useful
 *        for debugging purposes. When Debug is not set this will not be
 *        compiled in. Note that __write is overriden in iar_write.c,
 *        which makes printf work.
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

#ifndef _DEBUG_H
#define _DEBUG_H

extern void DEBUG_init(void);
extern void DEBUG_TxByte(uint8_t data);

static inline void led_set(uint8_t led)
{
    GPIO->P[DEBUG_PORT].DOUTSET = led;
}

static inline void led_clr(uint8_t led)
{
    GPIO->P[DEBUG_PORT].DOUTCLR = led;
}

#endif
