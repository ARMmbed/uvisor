/***************************************************************************//**
 * @file
 * @brief USB protocol stack library API for EFM32.
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
#ifndef __EM_USBH_H
#define __EM_USBH_H

#include "em_device.h"
#if defined( USB_PRESENT ) && ( USB_COUNT == 1 )
#include "em_usb.h"
#if defined( USB_HOST )

#ifdef __cplusplus
extern "C" {
#endif

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

extern USBH_Hc_TypeDef                  hcs[];
extern int                              USBH_attachRetryCount;
extern const USBH_AttachTiming_TypeDef  USBH_attachTiming[];
extern USBH_Init_TypeDef                USBH_initData;
extern volatile USBH_PortState_TypeDef  USBH_portStatus;

USB_Status_TypeDef USBH_CtlSendSetup(   USBH_Ep_TypeDef *ep );
USB_Status_TypeDef USBH_CtlSendData(    USBH_Ep_TypeDef *ep, uint16_t length );
USB_Status_TypeDef USBH_CtlReceiveData( USBH_Ep_TypeDef *ep, uint16_t length );

#if defined( USB_RAW_API )
int USBH_CtlRxRaw( uint8_t pid, USBH_Ep_TypeDef *ep, void *data, int byteCount );
int USBH_CtlTxRaw( uint8_t pid, USBH_Ep_TypeDef *ep, void *data, int byteCount );
#endif

void USBHEP_EpHandler(     USBH_Ep_TypeDef *ep, USB_Status_TypeDef result );
void USBHEP_CtrlEpHandler( USBH_Ep_TypeDef *ep, USB_Status_TypeDef result );
void USBHEP_TransferDone(  USBH_Ep_TypeDef *ep, USB_Status_TypeDef result );

__STATIC_INLINE uint16_t USBH_GetFrameNum( void )
{
  return USBHHAL_GetFrameNum();
}

__STATIC_INLINE bool USBH_FrameNumIsEven( void )
{
  return ( USBHHAL_GetFrameNum() & 1 ) == 0;
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* defined( USB_HOST ) */
#endif /* defined( USB_PRESENT ) && ( USB_COUNT == 1 ) */
#endif /* __EM_USBH_H      */
