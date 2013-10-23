/**************************************************************************//**
 * @file
 * @brief Bootloader autobaud functions.
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
#include "autobaud.h"

#define SAMPLE_MAX    5

volatile uint32_t currentSample = 0;
volatile uint32_t samples[SAMPLE_MAX];

#ifdef USART_OVERLAPS_WITH_BOOTLOADER
volatile uint8_t gpioEvent = 0;
#endif

/**************************************************************************//**
 * @brief
 *   GPIO interrupt handler. The purpose of this is to detect if there is a
 *   debugger attatched (Activity on SWDCLK - F1). If there is we should yield
 *   and let the debugger access the pin.
 * @note
 *   This is only required if the bootloader overlaps with the debug pins.
 *****************************************************************************/
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
void GPIO_IRQHandler(void)
{
  /* disable all interrupts */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;
  gpioEvent = 1;
#ifndef NDEBUG
  printf("Activity on SWDCLK. Suspending.\n");
#endif
}
#endif

/**************************************************************************//**
 * @brief
 *   AUTOBAUD_TIMER interrupt handler. This function stores the current value of the
 *   timer in the samples array.
 *****************************************************************************/
void TIMER_IRQHandler(void)
{
  uint32_t period;
  /* Clear CC1 flag */
#if AUTOBAUD_TIMER_CHANNEL == 0
  AUTOBAUD_TIMER->IFC = TIMER_IFC_CC0;
#elif AUTOBAUD_TIMER_CHANNEL == 1
  AUTOBAUD_TIMER->IFC = TIMER_IFC_CC1;
#elif AUTOBAUD_TIMER_CHANNEL == 2
  AUTOBAUD_TIMER->IFC = TIMER_IFC_CC2;
#else
#error Invalid timer channel.
#endif

  /* Store CC1 timer value in samples array */
  if (currentSample < SAMPLE_MAX)
  {
    period                 = AUTOBAUD_TIMER->CC[AUTOBAUD_TIMER_CHANNEL].CCV;
    samples[currentSample] = period;
    currentSample++;
  }
}

/**************************************************************************//**
 * @brief
 *   This function uses the samples array to estimate the period of a single
 *   bit flip.
 *
 * @return
 *   The number of HF clock periods (in 24.8 fixed point) per single bit flip.
 *****************************************************************************/
int AUTOBAUD_estimatePeriod(void)
{
  uint32_t minimumPeriod = UINT32_MAX;
  uint32_t i;
  uint32_t periodSum   = 0;
  uint32_t periodCount = 0;
  uint32_t diff;

  /* Find the smallest period. We only want to average the single-bit periods,
   * not any of the potential multi-bit periods */
  for (i = 1; i < currentSample; i++)
  {
    /* This calculation is split across two lines to avoid the compiler
     * complaining about samples being volatile. */
    diff  = samples[i];
    diff -= samples[i - 1];
    if (diff < minimumPeriod)
    {
      minimumPeriod = diff;
    }
  }

  /* average all samples within quarter a period of the smallest period, i.e.
   * the other single-bit periods */
  for (i = 1; i < currentSample; i++)
  {
    /* This calculation is split across two lines to avoid the compiler
     * complaining about samples being volatile. */
    diff  = samples[i];
    diff -= samples[i - 1];
    if (diff < minimumPeriod + (minimumPeriod >> 1))
    {
      periodSum += diff;
      periodCount++;
    }
  }

  /* Each sample is at least two periods. Shift by 7 instead of 8 to
   * get 24.8 format. */
  return (periodSum << 7) / periodCount;
}

/**************************************************************************//**
 * @brief
 *  This function uses AUTOBAUD_TIMER to estimate the needed CLKDIV needed for
 *  BOOTLOADER_USART. It does this by using compare channel
 *  AUTOBAUD_TIMER_CHANNEL and registering how many HF clock cycles occur
 *  between rising edges.
 *
 *  This assumes that AUTOBAUD_TIMER AUTOBAUD_TIMER_CHANNEL overlaps with the
 *  BOOTLOADER_USART RX pin.
 *
 * @return
 *   CLKDIV to use.
 *****************************************************************************/
int AUTOBAUD_sync(void)
{
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  /* Set up GPIO interrupts on falling edge on the SWDCLK. */
  /* If we see such an event, then there is a debugger attatched. */
  /* SWDIO is port F0 */
  GPIO->EXTIPSELL = GPIO_EXTIPSELL_EXTIPSEL0_PORTF;
  GPIO->EXTIFALL = 1;
  GPIO->IFC = 1;
  GPIO->IEN = 1;
#endif

  /* Set a high top value to avoid overflow */
  AUTOBAUD_TIMER->TOP = UINT32_MAX;

  /* Set up compare channel.
   * Trigger on rising edge and capture value */
  AUTOBAUD_TIMER->CC[AUTOBAUD_TIMER_CHANNEL].CTRL = TIMER_CC_CTRL_MODE_INPUTCAPTURE |
                                            TIMER_CC_CTRL_ICEDGE_RISING;

  /* Set up AUTOBAUD_TIMER to location AUTOBAUD_TIMER_LOCATION */
  AUTOBAUD_TIMER->ROUTE = AUTOBAUD_TIMER_LOCATION |
                  /* Enable CC channel 1 */
#if AUTOBAUD_TIMER_CHANNEL == 0
                  TIMER_ROUTE_CC0PEN;
#elif AUTOBAUD_TIMER_CHANNEL == 1
                  TIMER_ROUTE_CC1PEN;
#elif AUTOBAUD_TIMER_CHANNEL == 2
                  TIMER_ROUTE_CC2PEN;
#else
#error Invalid timer channel.
#endif

  /* Enable interrupts */
#if (AUTOBAUD_TIMER_IRQn < 32) && (GPIO_EVEN_IRQn < 32)
  NVIC->ISER[0] = (1 << AUTOBAUD_TIMER_IRQn) | (1 << GPIO_EVEN_IRQn);
#else
  NVIC_EnableIRQ(AUTOBAUD_TIMER_IRQn);
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
#endif
#endif

  /* Enable interrupt on channel capture */
#if AUTOBAUD_TIMER_CHANNEL == 0
  AUTOBAUD_TIMER->IEN = TIMER_IEN_CC0;
#elif AUTOBAUD_TIMER_CHANNEL == 1
  AUTOBAUD_TIMER->IEN = TIMER_IEN_CC1;
#elif AUTOBAUD_TIMER_CHANNEL == 2
  AUTOBAUD_TIMER->IEN = TIMER_IEN_CC2;
#else
#error Invalid timer channel.
#endif

  /* Start the timer */
  AUTOBAUD_TIMER->CMD = TIMER_CMD_START;

  /* Wait for interrupts in EM1 */
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

  /* Fill the sample buffer */
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  while ((currentSample < SAMPLE_MAX) && (!gpioEvent))
#else
  while (currentSample < SAMPLE_MAX)
#endif
  {
    __WFI();
  }

  /* Disable interrupts in Cortex */
#if (AUTOBAUD_TIMER_IRQn < 32) && (GPIO_EVEN_IRQn < 32)
  NVIC->ICER[0] = (1 << AUTOBAUD_TIMER_IRQn) | (1 << GPIO_EVEN_IRQn);
#else
  NVIC_DisableIRQ(AUTOBAUD_TIMER_IRQn);
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  NVIC_DisableIRQ(GPIO_EVEN_IRQn);
#endif
#endif

  /* Disable routing of TIMER. */
  AUTOBAUD_TIMER->ROUTE = _TIMER_ROUTE_RESETVALUE;

  /* If there has been a GPIO event, then loop here, so the debugger can
   * do it's stuff. */
#ifdef USART_OVERLAPS_WITH_BOOTLOADER
  while (gpioEvent) ;
#endif

  /* Use the collected samples to estimate the number of clock cycles per
   * bit in transfer */
  return AUTOBAUD_estimatePeriod();
}
