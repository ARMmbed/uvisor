/***************************************************************************//**
 * @file displayconfig.h
 * @brief Configuration file for DISPLAY device driver interface.
 * @author Energy Micro AS
 * @version 3.20.3
 *******************************************************************************
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
#ifndef __DISPLAYCONFIG_H
#define __DISPLAYCONFIG_H

/* Include support for the SHARP Memory LCD model LS013B7DH03 on the Zero Gecko
   kit (EFM32ZG_STK3200). */
#define INCLUDE_DISPLAY_SHARP_LS013B7DH03

#ifdef  INCLUDE_DISPLAY_SHARP_LS013B7DH03
#include "displayls013b7dh03config.h"
#include "displayls013b7dh03.h"
#endif

/**
 * Maximum number of display devices the display module is configured
 * to support. This number may be increased if the system includes more than
 * one display device. However, the number should be kept low in order to
 * save memory.
 */
#define DISPLAY_DEVICES_MAX   (1)

/**
 * Geometry of display device #0 in the system (i.e. ls013b7dh03 on the
 * Zero Gecko Kit (EFM32ZG_STK3200).
 * These defines can be used to declare static framebuffers in order to save
 * extra memory consumed by malloc.
 */
#define DISPLAY0_WIDTH    (LS013B7DH03_WIDTH)
#define DISPLAY0_HEIGHT   (LS013B7DH03_HEIGHT)


/**
 * Define all display device driver initialization functions here.
 */
#define DISPLAY_DEVICE_DRIVER_INIT_FUNCTIONS \
  {                                          \
    DISPLAY_Ls013b7dh03Init,                 \
    NULL                                     \
  }

#endif /* __DISPLAYCONFIG_H */
