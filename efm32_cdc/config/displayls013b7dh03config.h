/**************************************************************************//**
 * @file display_ls013b7dh03.h
 * @brief EFM32ZG_STK3200 specific configuration for the display driver for
 *        the Sharp Memory LCD model LS013B7DH03.
 * @author Energy Micro AS
 * @version 3.20.3
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

#ifndef _DISPLAY_LS013B7DH03_CONFIG_H_
#define _DISPLAY_LS013B7DH03_CONFIG_H_

/* Display device name. */
#define SHARP_MEMLCD_DEVICE_NAME   "Sharp LS013B7DH03 #1"


/* LCD and SPI GPIO pin connections on the EFM32ZG_STK3200. */
#define LCD_PORT_SCLK             (2)  /* = gpioPortC on EFM32ZG_STK3200 */
#define LCD_PIN_SCLK             (15)
#define LCD_PORT_SI               (3)  /* = gpioPortD on EFM32ZG_STK3200 */
#define LCD_PIN_SI                (7)
#define LCD_PORT_SCS              (4)  /* = gpioPortE on EFM32ZG_STK3200 */
#define LCD_PIN_SCS              (11)
#define LCD_PORT_EXTCOMIN         (4)  /* = gpioPortE on EFM32ZG_STK3200 */
#define LCD_PIN_EXTCOMIN         (10)
#define LCD_PORT_DISP             (0)  /* = gpioPortA on EFM32ZG_STK3200 */
#define LCD_PIN_DISP              (8)
#define LCD_PORT_EXTMODE          (0)  /* = gpioPortA on EFM32ZG_STK3200 */
#define LCD_PIN_EXTMODE           (0)


/*
 * Select how LCD polarity inversion should be handled:
 *
 * If POLARITY_INVERSION_EXTCOMIN is defined, the EXTMODE pin is set to HIGH,
 * and the polarity inversion is armed for every rising edge of the EXTCOMIN
 * pin. The actual polarity inversion is triggered at the next transision of
 * SCS. This mode is recommended because it causes less CPU and SPI load than
 * the alternative mode, see below.
 * If POLARITY_INVERSION_EXTCOMIN is undefined, the EXTMODE pin is set to LOW,
 * and the polarity inversion is toggled by sending an SPI command. This mode
 * causes more CPU and SPI load than using the EXTCOMIN pin mode.
 */
#define POLARITY_INVERSION_EXTCOMIN

/* Define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE if you want the PAL
 * (Platform Abstraction Layer interface) to automatically toggle the EXTCOMIN
 *  pin.
 */
#define POLARITY_INVERSION_EXTCOMIN_PAL_AUTO_TOGGLE


#endif /* _DISPLAY_LS013B7DH03_CONFIG_H_ */
