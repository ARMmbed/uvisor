/**************************************************************************//**
 * @file
 * @brief SWO Trace API (for eAProfiler)
 * @author Energy Micro AS
 * @version 3.20.2
 ******************************************************************************
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
#ifndef __BSP_TRACE_H
#define __BSP_TRACE_H

#include "em_device.h"
#if (defined(BSP_ETM_TRACE) && defined( ETM_PRESENT )) || defined( GPIO_ROUTE_SWOPEN )

#include <stdint.h>
#include <stdbool.h>
#include "em_msc.h"
#include "traceconfig.h"

/***************************************************************************//**
 * @addtogroup BSP
 * @{
 ******************************************************************************/
/***************************************************************************//**
 * @addtogroup BSPCOMMON API common for all kits
 * @{
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_ETM_TRACE) && defined( ETM_PRESENT )
void BSP_TraceEtmSetup(void);
#endif

#if defined( GPIO_ROUTE_SWOPEN )
bool BSP_TraceProfilerSetup(void);
void BSP_TraceSwoSetup(void);
#endif

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#define USER_PAGE    0x0FE00000UL /* Address to flash memory */
/** @endcond */

/**************************************************************************//**
 * @brief Set or clear word in user page which enables or disables SWO
 *        in BSP_TraceProfilerSetup. If BSP_TraceProfilerEnable(false) has been run,
 *        no example project will enable SWO trace.
 * @param[in] enable
 * @note Add "em_msc.c" to build to use this function.
 *****************************************************************************/
__STATIC_INLINE void BSP_TraceProfilerEnable(bool enable)
{
  uint32_t          data;
  volatile uint32_t *userpage = (uint32_t *) USER_PAGE;

  /* Check that configuration needs to change */
  data = *userpage;
  if (enable)
  {
    if (data == 0xFFFFFFFF)
    {
      return;
    }
  }
  else
  {
    if (data == 0x00000000)
    {
      return;
    }
  }

  /* Initialize MSC */
  MSC_Init();

  /* Write enable or disable trigger word into flash */
  if (enable)
  {
    data = 0xFFFFFFFF;
    MSC_ErasePage((uint32_t *) USER_PAGE);
    MSC_WriteWord((uint32_t *) USER_PAGE, (void *) &data, 4);
  }
  else
  {
    data = 0x00000000;
    MSC_ErasePage((uint32_t *) USER_PAGE);
    MSC_WriteWord((uint32_t *) USER_PAGE, (void *) &data, 4);
  }
}

#ifdef __cplusplus
}
#endif

/** @} (end group BSP) */
/** @} (end group BSP) */

#endif  /* (defined(BSP_ETM_TRACE) && defined( ETM_PRESENT )) || defined( GPIO_ROUTE_SWOPEN ) */
#endif  /* __BSP_TRACE_H */
