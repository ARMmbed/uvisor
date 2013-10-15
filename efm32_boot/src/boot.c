/**************************************************************************//**
 * @file
 * @brief Boot Loader
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

#include "xmodem.h"
#include "em_device.h"
#include "boot.h"
#include "compiler.h"
#include "config.h"

#ifndef NDEBUG
bool printedPC = false;
#endif

/**************************************************************************//**
 * @brief Checks to see if the reset vector of the application is valid
 * @return false if the firmware is not valid, true if it is.
 *****************************************************************************/
bool BOOT_checkFirmwareIsValid(void)
{
  uint32_t pc;
  pc = *((uint32_t *) BOOTLOADER_SIZE + 1);
#ifndef NDEBUG
  if (!printedPC)
  {
    printedPC = true;
    printf("Application Reset vector = 0x%x \r\n", pc);
  }
#endif
  if (pc < MAX_SIZE_OF_FLASH)
    return true;
  return false;
}

/**************************************************************************//**
 * @brief This function sets up the Cortex M-3 with a new SP and PC.
 *****************************************************************************/
#if defined ( __CC_ARM   )
__asm void BOOT_jump(uint32_t sp, uint32_t pc)
{
  /* Set new MSP, PSP based on SP (r0)*/
  msr msp, r0
  msr psp, r0

  /* Jump to PC (r1)*/
  mov pc, r1
}
#else
__ramfunc void BOOT_jump(uint32_t sp, uint32_t pc)
{
  (void) sp;
  (void) pc;
  /* Set new MSP, PSP based on SP (r0)*/
  __asm("msr msp, r0");
  __asm("msr psp, r0");

  /* Jump to PC (r1)*/
  __asm("mov pc, r1");
}
#endif

/**************************************************************************//**
 * @brief Boots the application
 *****************************************************************************/
__ramfunc void BOOT_boot(void)
{
  uint32_t pc, sp;

  /* Reset registers */
  /* Clear all interrupts set. */
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;
  RTC->CTRL  = _RTC_CTRL_RESETVALUE;
  RTC->COMP0 = _RTC_COMP0_RESETVALUE;
  RTC->IEN   = _RTC_IEN_RESETVALUE;
  /* Reset GPIO settings */
  GPIO->P[5].MODEL = _GPIO_P_MODEL_RESETVALUE;
  /* Disable RTC clock */
  CMU->LFACLKEN0 = _CMU_LFACLKEN0_RESETVALUE;
  CMU->LFCLKSEL  = _CMU_LFCLKSEL_RESETVALUE;
  /* Disable LFRCO */
  CMU->OSCENCMD = CMU_OSCENCMD_LFRCODIS;
  /* Disable LE interface */
  CMU->HFCORECLKEN0 = _CMU_HFCORECLKEN0_RESETVALUE;
  /* Reset clocks */
  CMU->HFPERCLKDIV = _CMU_HFPERCLKDIV_RESETVALUE;
  CMU->HFPERCLKEN0 = _CMU_HFPERCLKEN0_RESETVALUE;

  /* Set new vector table */
  SCB->VTOR = (uint32_t) BOOTLOADER_SIZE;
  /* Read new SP and PC from vector table */
  sp = *((uint32_t *) BOOTLOADER_SIZE);
  pc = *((uint32_t *) BOOTLOADER_SIZE + 1);

  BOOT_jump(sp, pc);
}
