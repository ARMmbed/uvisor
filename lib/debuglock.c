/**************************************************************************//**
 * @file
 * @brief Boot Loader debug lock
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
#include <stdio.h>
#include <stdbool.h>

#include "debuglock.h"
#include "em_device.h"

#include "flash.h"
#include "config.h"

/*************************************************************************//**
 * This is the SWD debug sequence.
 * The first three lines is a simple initialization of the interface and the
 * last sets the powerup request bit, which enables debug mode.
 * By enabling debug mode, we can write to the Debug lock word.
 ****************************************************************************/
uint8_t _DEBUGLOCK_swData[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00,
  0xa5, 0x00, 0x00, 0x00, 0x00, 0x00, /* read DP-ID - init sequence ends after this */
  0xa9, 0x00, 0x00, 0x00, 0x00, 0x0a  /* ctrlstat = 0x50000000 (wake up) */
};

/*************************************************************************//**
 * @brief
 *   This functions drives a bit on the SW debug interface by bit-banging
 *   the GPIO->ROUTE register and using the external pull-ups.
 ****************************************************************************/
__ramfunc __INLINE void _DEBUGLOCK_driveBit(int bit)
{
  int i;
  if (bit)
  {
    /* SWDIO high */
    GPIO->ROUTE |= GPIO_ROUTE_SWDIOPEN;
  }
  else
  {
    /* SWDIO low */
    GPIO->ROUTE &= ~(GPIO_ROUTE_SWDIOPEN);
  }
  /* Toggle clock. */
  /* Set SWDCLK high */
  GPIO->ROUTE |= GPIO_ROUTE_SWCLKPEN;
  GPIO->P[5].MODEL = GPIO_P_MODEL_MODE0_INPUT;
  /* Small delay, to make sure that any external line is charged. */
  for (i=0; i < 1000; i++);
  /* Set SWDCLK low */
  GPIO->ROUTE &= ~(GPIO_ROUTE_SWCLKPEN);
  GPIO->P[5].MODEL = GPIO_P_MODEL_MODE0_DISABLED;
}

/*************************************************************************//**
 * @brief
 *   This functions enables the debug interface by bit-banging a SWD init
 *   sequence on the debug interface.
 ****************************************************************************/
__ramfunc void DEBUGLOCK_startDebugInterface(void)
{
  int bit, byte;
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  uint32_t tmp;
  tmp = GPIO->P[5].MODEL;
#endif
  GPIO->P[5].MODEL = 0;
  GPIO->ROUTE = 0;

  for (byte = 0; byte < sizeof(_DEBUGLOCK_swData); byte++)
  {
    for (bit = 0; bit < 8; bit++)
    {
      _DEBUGLOCK_driveBit((_DEBUGLOCK_swData[byte] >> bit) & 1);
    }
  }
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  GPIO->P[5].MODEL = tmp;
#endif
}

/*************************************************************************//**
 * @brief
 *   This function locks the debug interface by first enabling debug mode by
 *   bit-banging a SWD init sequence on the SWD interface and then setting
 *   the Debug lock bit in the flash.
 * @return
 *   Returns true if the DLW has been sucessfully set, false otherwise.
 ****************************************************************************/
__ramfunc bool DEBUGLOCK_lock(void)
{
  /* Start debug interface. */
  DEBUGLOCK_startDebugInterface();
  /* Set the debug lock word */
  FLASH_writeWord(DEBUG_LOCK_WORD, 0x0);
  /* Verify sucessful write by checking the DLW. */
  if (*((uint32_t *) DEBUG_LOCK_WORD) == 0x0)
  {
    return true;
  }
  return false;
}
