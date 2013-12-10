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
#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "bsp.h"

#if defined( BSP_GPIO_LEDS )
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

typedef struct
{
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} tLedArray;

static const tLedArray ledArray[ BSP_NO_OF_LEDS ] = BSP_GPIO_LEDARRAY_INIT;

int BSP_LedsInit(void)
{
  int i;

  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  for ( i=0; i<BSP_NO_OF_LEDS; i++ )
  {
    GPIO_PinModeSet(ledArray[i].port, ledArray[i].pin, gpioModePushPull, 0);
  }
  return BSP_STATUS_OK;
}

uint32_t BSP_LedsGet(void)
{
  int i;
  uint32_t retVal, mask;

  for ( i=0, retVal=0, mask=0x1; i<BSP_NO_OF_LEDS; i++, mask <<= 1 )
  {
    if (GPIO_PinOutGet(ledArray[i].port, ledArray[i].pin))
      retVal |= mask;
  }
  return retVal;
}

int BSP_LedsSet(uint32_t leds)
{
  int i;
  uint32_t mask;

  for ( i=0, mask=0x1; i<BSP_NO_OF_LEDS; i++, mask <<= 1 )
  {
    if ( leds & mask )
      GPIO_PinOutSet(ledArray[i].port, ledArray[i].pin);
    else
      GPIO_PinOutClear(ledArray[i].port, ledArray[i].pin);
  }
  return BSP_STATUS_OK;
}

int BSP_LedClear(int ledNo)
{
  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    GPIO_PinOutClear(ledArray[ledNo].port, ledArray[ledNo].pin);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

int BSP_LedGet(int ledNo)
{
  int retVal = BSP_STATUS_ILLEGAL_PARAM;

  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    retVal = (int)GPIO_PinOutGet(ledArray[ledNo].port, ledArray[ledNo].pin);
  }
  return retVal;
}

int BSP_LedSet(int ledNo)
{
  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    GPIO_PinOutSet(ledArray[ledNo].port, ledArray[ledNo].pin);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

int BSP_LedToggle(int ledNo)
{
  if ((ledNo >= 0) && (ledNo < BSP_NO_OF_LEDS))
  {
    GPIO_PinOutToggle(ledArray[ledNo].port, ledArray[ledNo].pin);
    return BSP_STATUS_OK;
  }
  return BSP_STATUS_ILLEGAL_PARAM;
}

/** @endcond */
#endif  /* BSP_GPIO_LEDS */
