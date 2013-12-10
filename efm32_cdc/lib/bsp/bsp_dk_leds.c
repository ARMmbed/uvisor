/***************************************************************************//**
 * @file
 * @brief Board support package API for GPIO leds on STK's.
 * @author Energy Micro AS
 * @version 3.20.2
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2012 Energy Micro AS, http://www.energymicro.com</b>
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
#include "bsp.h"

#if defined( BSP_DK_LEDS )

/***************************************************************************//**
 * @addtogroup BSPCOMMON API common for all kits
 * @{
 ******************************************************************************/

/**************************************************************************//**
 * @brief Initialize LED drivers.
 * @note  LED's are initially turned off.
 * @return
 *   @ref BSP_STATUS_OK
 *****************************************************************************/
int BSP_LedsInit(void)
{
  BSP_RegisterWrite(BSP_LED_PORT, 0);
  return BSP_STATUS_OK;
}

/**************************************************************************//**
 * @brief Get status of all LED's.
 * @return
 *   Bitmask with current status for all LED's.
 *****************************************************************************/
uint32_t BSP_LedsGet(void)
{
  return BSP_RegisterRead(BSP_LED_PORT) & BSP_LED_MASK;
}

/**************************************************************************//**
 * @brief Update all LED's.
 * @param[in] leds Bitmask representing new status for all LED's. A 1 turns
 *                 a LED on, a 0 turns a LED off.
 * @return
 *   @ref BSP_STATUS_OK
 *****************************************************************************/
int BSP_LedsSet(uint32_t leds)
{
  BSP_RegisterWrite(BSP_LED_PORT, leds & BSP_LED_MASK);
  return BSP_STATUS_OK;
}

/**************************************************************************//**
 * @brief Turn off a single LED.
 * @param[in] ledNo The number of the LED (counting from zero) to turn off.
 * @return
 *   @ref BSP_STATUS_OK or @ref BSP_STATUS_ILLEGAL_PARAM if illegal LED number.
 *****************************************************************************/
int BSP_LedClear(int ledNo)
{
  uint32_t tmp;

  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    tmp = BSP_RegisterRead(BSP_LED_PORT) & BSP_LED_MASK;
    tmp &= ~( 1 << ledNo );
    BSP_RegisterWrite(BSP_LED_PORT, tmp);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

/**************************************************************************//**
 * @brief Get current status of a single LED.
 * @param[in] ledNo The number of the LED (counting from zero) to check.
 * @return
 *   1 if LED is on, 0 if LED is off, @ref BSP_STATUS_ILLEGAL_PARAM if illegal
 *   LED number.
 *****************************************************************************/
int BSP_LedGet(int ledNo)
{
  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    if ( BSP_RegisterRead(BSP_LED_PORT) & BSP_LED_MASK & (1 << ledNo) )
      return 1;

    return 0;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

/**************************************************************************//**
 * @brief Turn on a single LED.
 * @param[in] ledNo The number of the LED (counting from zero) to turn on.
 * @return
 *   @ref BSP_STATUS_OK or @ref BSP_STATUS_ILLEGAL_PARAM if illegal LED number.
 *****************************************************************************/
int BSP_LedSet(int ledNo)
{
  uint32_t tmp;

  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    tmp = BSP_RegisterRead(BSP_LED_PORT) & BSP_LED_MASK;
    tmp |= 1 << ledNo;
    BSP_RegisterWrite(BSP_LED_PORT, tmp);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

/**************************************************************************//**
 * @brief Toggle a single LED.
 * @param[in] ledNo The number of the LED (counting from zero) to toggle.
 * @return
 *   @ref BSP_STATUS_OK or @ref BSP_STATUS_ILLEGAL_PARAM if illegal LED number.
 *****************************************************************************/
int BSP_LedToggle(int ledNo)
{
  uint32_t tmp;

  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    tmp = BSP_RegisterRead(BSP_LED_PORT) & BSP_LED_MASK;
    tmp ^= 1 << ledNo;
    BSP_RegisterWrite(BSP_LED_PORT, tmp);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

/** @} */
#endif  /* BSP_DK_LEDS */
