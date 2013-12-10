/***************************************************************************//**
 * @file
 * @brief Low Energy Sensor (LESENSE) example configuration file.
 * @author Energy Micro AS
 * @version 3.20.3
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

#include "em_lesense.h"

/***************************************************************************//**
 * @addtogroup Drivers
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CapSense
 * @{
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * Macro definitions
 *****************************************************************************/
#define CAPLESENSE_SENSITIVITY_OFFS    1U
#define CAPLESENSE_NUMOF_SLIDERS       4                          /**< Number of sliders */
#define CAPLESENSE_ACMP_VDD_SCALE      LESENSE_ACMP_VDD_SCALE     /**< Upper voltage threshold */

#define CAPLESENSE_SLIDER_PORT0        gpioPortC                  /**< Slider Port. GPIO Port C */
#define CAPLESENSE_SLIDER0_PORT        CAPLESENSE_SLIDER_PORT0      /**< Slider 0 Port. GPIO Port C */
#define CAPLESENSE_SLIDER0_PIN         8UL                        /**< Slider 0 Pin 8 */
#define CAPLESENSE_SLIDER1_PORT        CAPLESENSE_SLIDER_PORT0      /**< Slider 1 Port. GPIO Port C */
#define CAPLESENSE_SLIDER1_PIN         9UL                        /**< Slider 1 Pin 9 */
#define CAPLESENSE_SLIDER2_PORT        CAPLESENSE_SLIDER_PORT0      /**< Slider 2 Port. GPIO Port C */
#define CAPLESENSE_SLIDER2_PIN         10UL                       /**< Slider 2 Pin 10 */
#define CAPLESENSE_SLIDER3_PORT        CAPLESENSE_SLIDER_PORT0      /**< Slider 3 Port. GPIO Port C */
#define CAPLESENSE_SLIDER3_PIN         11UL                       /**< Slider 3 Pin 11 */


#define CAPLESENSE_CHANNEL_INT        (LESENSE_IF_CH8 | LESENSE_IF_CH9 | LESENSE_IF_CH10 | LESENSE_IF_CH11)
#define LESENSE_CHANNELS        16  /**< Number of channels for the Low Energy Sensor Interface. */

#define SLIDER_PART0_CHANNEL    8   /**< Touch slider channel Part 0 */
#define SLIDER_PART1_CHANNEL    9   /**< Touch slider channel Part 1 */
#define SLIDER_PART2_CHANNEL    10  /**< Touch slider channel Part 2 */
#define SLIDER_PART3_CHANNEL    11  /**< Touch slider channel Part 3 */

/** Upper voltage threshold. */
#define LESENSE_ACMP_VDD_SCALE    0x37U


#define LESENSE_CAPSENSE_CH_IN_USE {\
/*  Ch0,   Ch1,   Ch2,   Ch3,   Ch4,   Ch5,   Ch6,   Ch7    */\
  false, false, false, false, false, false, false, false,\
/*  Ch8,   Ch9,   Ch10,  Ch11,  Ch12,  Ch13,  Ch14,  Ch15   */\
  true,  true,  true,  true,  false, false, false, false\
}

/** Configuration for capacitive sense channels in sense mode. */
#define LESENSE_CAPSENSE_CH_CONF_SENSE                                                                   \
  {                                                                                                      \
    true,                     /* Enable scan channel. */                                                 \
    true,                     /* Enable the assigned pin on scan channel. */                             \
    false,                    /* Disable interrupts on channel. */                                       \
    lesenseChPinExDis,        /* GPIO pin is disabled during the excitation period. */                   \
    lesenseChPinIdleDis,      /* GPIO pin is disabled during the idle period. */                         \
    false,                    /* Don't use alternate excitation pins for excitation. */                  \
    false,                    /* Disabled to shift results from this channel to the decoder register. */ \
    false,                    /* Disabled to invert the scan result bit. */                              \
    true,                     /* Enabled to store counter value in the result buffer. */                 \
    lesenseClkLF,             /* Use the LF clock for excitation timing. */                              \
    lesenseClkLF,             /* Use the LF clock for sample timing. */                                  \
    0x00U,                    /* Excitation time is set to 0 excitation clock cycles. */                 \
    0x0FU,                    /* Sample delay is set to 15(+1) sample clock cycles. */                   \
    0x00U,                    /* Measure delay is set to 0 excitation clock cycles.*/                    \
    LESENSE_ACMP_VDD_SCALE,   /* ACMP threshold has been set to LESENSE_ACMP_VDD_SCALE. */               \
    lesenseSampleModeCounter, /* ACMP will be used in comparison. */                                     \
    lesenseSetIntLevel,       /* Interrupt is generated if the sensor triggers. */                       \
    0x00U,                    /* Counter threshold has been set to 0x00. */                              \
    lesenseCompModeLess       /* Compare mode has been set to trigger interrupt on "less". */            \
  }

/** Configuration for capacitive sense channels in sleep mode. */
#define LESENSE_CAPSENSE_CH_CONF_SLEEP                                                                   \
  {                                                                                                      \
    true,                     /* Enable scan channel. */                                                 \
    true,                     /* Enable the assigned pin on scan channel. */                             \
    true,                     /* Enable interrupts on channel. */                                        \
    lesenseChPinExDis,        /* GPIO pin is disabled during the excitation period. */                   \
    lesenseChPinIdleDis,      /* GPIO pin is disabled during the idle period. */                         \
    false,                    /* Don't use alternate excitation pins for excitation. */                  \
    false,                    /* Disabled to shift results from this channel to the decoder register. */ \
    false,                    /* Disabled to invert the scan result bit. */                              \
    true,                     /* Enabled to store counter value in the result buffer. */                 \
    lesenseClkLF,             /* Use the LF clock for excitation timing. */                              \
    lesenseClkLF,             /* Use the LF clock for sample timing. */                                  \
    0x00U,                    /* Excitation time is set to 0 excitation clock cycles. */                 \
    0x01U,                    /* Sample delay is set to 1(+1) sample clock cycles. */                    \
    0x00U,                    /* Measure delay is set to 0 excitation clock cycles.*/                    \
    LESENSE_ACMP_VDD_SCALE,   /* ACMP threshold has been set to LESENSE_ACMP_VDD_SCALE. */               \
    lesenseSampleModeCounter, /* Counter will be used in comparison. */                                  \
    lesenseSetIntLevel,       /* Interrupt is generated if the sensor triggers. */                       \
    0x0EU,                    /* Counter threshold has been set to 0x0E. */                              \
    lesenseCompModeLess       /* Compare mode has been set to trigger interrupt on "less". */            \
  }

/** Configuration for disabled channels. */
#define LESENSE_DISABLED_CH_CONF                                                                         \
  {                                                                                                      \
    false,                    /* Disable scan channel. */                                                \
    false,                    /* Disable the assigned pin on scan channel. */                            \
    false,                    /* Disable interrupts on channel. */                                       \
    lesenseChPinExDis,        /* GPIO pin is disabled during the excitation period. */                   \
    lesenseChPinIdleDis,      /* GPIO pin is disabled during the idle period. */                         \
    false,                    /* Don't use alternate excitation pins for excitation. */                  \
    false,                    /* Disabled to shift results from this channel to the decoder register. */ \
    false,                    /* Disabled to invert the scan result bit. */                              \
    false,                    /* Disabled to store counter value in the result buffer. */                \
    lesenseClkLF,             /* Use the LF clock for excitation timing. */                              \
    lesenseClkLF,             /* Use the LF clock for sample timing. */                                  \
    0x00U,                    /* Excitation time is set to 5(+1) excitation clock cycles. */             \
    0x00U,                    /* Sample delay is set to 7(+1) sample clock cycles. */                    \
    0x00U,                    /* Measure delay is set to 0 excitation clock cycles.*/                    \
    0x00U,                    /* ACMP threshold has been set to 0. */                                    \
    lesenseSampleModeCounter, /* ACMP output will be used in comparison. */                              \
    lesenseSetIntNone,        /* No interrupt is generated by the channel. */                            \
    0x00U,                    /* Counter threshold has been set to 0x01. */                              \
    lesenseCompModeLess       /* Compare mode has been set to trigger interrupt on "less". */            \
  }

/** Configuration for scan in sense mode. */
#define LESENSE_CAPSENSE_SCAN_CONF_SENSE                 \
  {                                                      \
    {                                                    \
      LESENSE_DISABLED_CH_CONF,        /* Channel 0. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 1. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 2. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 3. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 4. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 5. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 6. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 7. */  \
      LESENSE_CAPSENSE_CH_CONF_SENSE,  /* Channel 8. */  \
      LESENSE_CAPSENSE_CH_CONF_SENSE,  /* Channel 9. */  \
      LESENSE_CAPSENSE_CH_CONF_SENSE,  /* Channel 10. */ \
      LESENSE_CAPSENSE_CH_CONF_SENSE,  /* Channel 11. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 12. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 13. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 14. */ \
      LESENSE_DISABLED_CH_CONF         /* Channel 15. */ \
    }                                                    \
  }

/** Configuration for scan in sleep mode. */
#define LESENSE_CAPSENSE_SCAN_CONF_SLEEP                 \
  {                                                      \
    {                                                    \
      LESENSE_DISABLED_CH_CONF,        /* Channel 0. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 1. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 2. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 3. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 4. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 5. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 6. */  \
      LESENSE_DISABLED_CH_CONF,        /* Channel 7. */  \
      LESENSE_CAPSENSE_CH_CONF_SLEEP,  /* Channel 8. */  \
      LESENSE_CAPSENSE_CH_CONF_SLEEP,  /* Channel 9. */  \
      LESENSE_CAPSENSE_CH_CONF_SLEEP,  /* Channel 10. */ \
      LESENSE_CAPSENSE_CH_CONF_SLEEP,  /* Channel 11. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 12. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 13. */ \
      LESENSE_DISABLED_CH_CONF,        /* Channel 14. */ \
      LESENSE_DISABLED_CH_CONF         /* Channel 15. */ \
    }                                                    \
  }

#ifdef __cplusplus
}
#endif

/** @} (end group CapSense) */
/** @} (end group Drivers) */
