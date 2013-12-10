/**************************************************************************//**
 * @file
 * @brief USB protocol stack library, USB chapter 9 command handler.
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
#include "em_device.h"
#if defined( USB_PRESENT ) && ( USB_COUNT == 1 )
#include "em_usb.h"
#if defined( USB_DEVICE )

#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbd.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static USB_Status_TypeDef ClearFeature     ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef GetConfiguration ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef GetDescriptor    ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef GetInterface     ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef GetStatus        ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef SetAddress       ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef SetConfiguration ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef SetFeature       ( USBD_Device_TypeDef *pDev );
static USB_Status_TypeDef SetInterface     ( USBD_Device_TypeDef *pDev );

static uint32_t txBuf;

int USBDCH9_SetupCmd( USBD_Device_TypeDef *device )
{
  int status;
  USB_Setup_TypeDef *p = device->setup;

  /* Vendor unique, Class or Standard setup commands override ? */
  if ( device->callbacks->setupCmd )
  {
    status = device->callbacks->setupCmd( p );

    if ( status != USB_STATUS_REQ_UNHANDLED )
    {
      return status;
    }
  }

  status = USB_STATUS_REQ_ERR;

  if ( p->Type == USB_SETUP_TYPE_STANDARD )
  {
    switch ( p->bRequest )
    {
      case GET_STATUS:
        status = GetStatus( device );
        break;

      case CLEAR_FEATURE:
        status = ClearFeature( device );
        break;

      case SET_FEATURE:
        status = SetFeature( device );
        break;

      case SET_ADDRESS:
        status = SetAddress( device );
        break;

      case GET_DESCRIPTOR:
        status = GetDescriptor( device );
        break;

      case GET_CONFIGURATION:
        status = GetConfiguration( device );
        break;

      case SET_CONFIGURATION:
        status = SetConfiguration( device );
        break;

      case GET_INTERFACE:
        status = GetInterface( device );
        break;

      case SET_INTERFACE:
        status = SetInterface( device );
        break;

      case SYNCH_FRAME:     /* Synch frame is for isochronous endpoints */
      case SET_DESCRIPTOR:  /* Set descriptor is optional */
      default:
        break;
    }
  }

  return status;
}

static USB_Status_TypeDef ClearFeature( USBD_Device_TypeDef *pDev )
{
  USBD_Ep_TypeDef *ep;
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( p->wLength != 0 )
  {
    return USB_STATUS_REQ_ERR;
  }

  switch ( p->Recipient )
  {
    case USB_SETUP_RECIPIENT_DEVICE:
      if ( ( p->wIndex == 0                                 ) && /* ITF no. 0 */
           ( p->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP  ) &&
           ( ( pDev->state == USBD_STATE_ADDRESSED     ) ||
             ( pDev->state == USBD_STATE_CONFIGURED    )    )    )
      {
        /* Remote wakeup feature clear */
        if ( pDev->configDescriptor->bmAttributes & CONFIG_DESC_BM_REMOTEWAKEUP )
          {
          /* The device is capable of signalling remote wakeup */
          pDev->remoteWakeupEnabled = false;
          retVal = USB_STATUS_OK;
          }
      }
      break;

    case USB_SETUP_RECIPIENT_ENDPOINT:
      ep = USBD_GetEpFromAddr( p->wIndex & 0xFF );
      if ( ep )
      {
        if ( ( ep->num > 0                            ) &&
             ( p->wValue == USB_FEATURE_ENDPOINT_HALT ) &&
             ( pDev->state == USBD_STATE_CONFIGURED   )    )
        {
          retVal = USBDHAL_UnStallEp( ep );
        }
      }
  }

  return retVal;
}

static USB_Status_TypeDef GetConfiguration( USBD_Device_TypeDef *pDev )
{
  USB_Setup_TypeDef *p = pDev->setup;
  uint8_t *pConfigValue = (uint8_t*)&txBuf;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( ( p->wIndex    != 0                          ) ||
       ( p->wLength   != 1                          ) ||
       ( p->wValue    != 0                          ) ||
       ( p->Direction != USB_SETUP_DIR_IN           ) ||
       ( p->Recipient != USB_SETUP_RECIPIENT_DEVICE )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  if ( pDev->state == USBD_STATE_ADDRESSED )
  {
    *pConfigValue = 0;
    USBD_Write( 0, pConfigValue, 1, NULL );
    retVal = USB_STATUS_OK;
  }

  else if ( pDev->state == USBD_STATE_CONFIGURED )
  {
    USBD_Write( 0, &pDev->configurationValue, 1, NULL );
    retVal = USB_STATUS_OK;
  }

  return retVal;
}

static USB_Status_TypeDef GetDescriptor( USBD_Device_TypeDef *pDev )
{
  int index;
  uint16_t length = 0;
  const void *data = NULL;
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( ( p->Recipient != USB_SETUP_RECIPIENT_DEVICE ) ||
       ( p->Direction != USB_SETUP_DIR_IN           )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  index = p->wValue & 0xFF;
  switch ( p->wValue >> 8 )
  {
    case USB_DEVICE_DESCRIPTOR :
      if ( index != 0 )
      {
        break;
      }
      data   = pDev->deviceDescriptor;
      length = pDev->deviceDescriptor->bLength;
      break;

    case USB_CONFIG_DESCRIPTOR :
      if ( index != 0 )
      {
        break;
      }
      data   = pDev->configDescriptor;
      length = pDev->configDescriptor->wTotalLength;
      break;

    case USB_STRING_DESCRIPTOR :
      if ( index < pDev->numberOfStrings )
      {
        USB_StringDescriptor_TypeDef *s;
        s = ((USB_StringDescriptor_TypeDef**)pDev->stringDescriptors)[index];

        data   = s;
        length = s->len;
      }
      break;
  }

  if ( length )
  {
    USBD_Write( 0, (void*)data, EFM32_MIN( length, p->wLength ), NULL );
    retVal = USB_STATUS_OK;
  }

  return retVal;
}

static USB_Status_TypeDef GetInterface( USBD_Device_TypeDef *pDev )
{
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;
  uint8_t *pAlternateSetting = (uint8_t*)&txBuf;

  /* There is currently support for one interface and no alternate settings */

  if ( ( p->wIndex      != 0                             ) ||
       ( p->wLength     != 1                             ) ||
       ( p->wValue      != 0                             ) ||
       ( p->Direction   != USB_SETUP_DIR_IN              ) ||
       ( p->Recipient   != USB_SETUP_RECIPIENT_INTERFACE )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  if ( pDev->state == USBD_STATE_CONFIGURED )
  {
    *pAlternateSetting = 0;
    USBD_Write( 0, pAlternateSetting, 1, NULL );
    retVal = USB_STATUS_OK;
  }

  return retVal;
}

static USB_Status_TypeDef GetStatus( USBD_Device_TypeDef *pDev )
{
  USBD_Ep_TypeDef *ep;
  USB_Setup_TypeDef *p = pDev->setup;
  uint16_t *pStatus = (uint16_t*)&txBuf;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( ( p->wValue    != 0                ) ||
       ( p->wLength   != 2                ) ||
       ( p->Direction != USB_SETUP_DIR_IN )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  switch ( p->Recipient )
  {
    case USB_SETUP_RECIPIENT_DEVICE:
      if ( ( p->wIndex == 0                              ) &&
           ( ( pDev->state == USBD_STATE_ADDRESSED  ) ||
             ( pDev->state == USBD_STATE_CONFIGURED )    )    )
      {
        *pStatus = 0;

        /* Remote wakeup feature status */
        if ( pDev->remoteWakeupEnabled )
          *pStatus |= REMOTE_WAKEUP_ENABLED;

        /* Current self/bus power status */
        if ( pDev->callbacks->isSelfPowered != NULL )
        {
          if ( pDev->callbacks->isSelfPowered() )
          {
            *pStatus |= DEVICE_IS_SELFPOWERED;
          }
        }
        else
        {
          if ( pDev->configDescriptor->bmAttributes & CONFIG_DESC_BM_SELFPOWERED )
          {
            *pStatus |= DEVICE_IS_SELFPOWERED;
          }
        }

        USBD_Write( 0, pStatus, 2, NULL );
        retVal = USB_STATUS_OK;
      }
      break;

    case USB_SETUP_RECIPIENT_INTERFACE:
      if ( ( p->wIndex == 0                              ) &&
           ( ( pDev->state == USBD_STATE_ADDRESSED  ) ||
             ( pDev->state == USBD_STATE_CONFIGURED )    )    )
      {
        *pStatus = 0;
        USBD_Write( 0, pStatus, 2, NULL );
        retVal = USB_STATUS_OK;
      }
      break;

    case USB_SETUP_RECIPIENT_ENDPOINT:
      ep = USBD_GetEpFromAddr( p->wIndex & 0xFF );
      if ( ep )
      {
        if ( ( ep->num > 0                          ) &&
             ( pDev->state == USBD_STATE_CONFIGURED )    )
        {
          /* Send 2 bytes w/halt status for endpoint */
          retVal = USBDHAL_GetStallStatusEp( ep, pStatus );
          if ( retVal == USB_STATUS_OK )
          {
            USBD_Write( 0, pStatus, 2, NULL );
          }
        }
      }
      break;
  }

  return retVal;
}

static USB_Status_TypeDef SetAddress( USBD_Device_TypeDef *pDev )
{
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( ( p->wIndex    != 0                          ) ||
       ( p->wLength   != 0                          ) ||
       ( p->wValue    >  127                        ) ||
       ( p->Recipient != USB_SETUP_RECIPIENT_DEVICE )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  if ( pDev->state == USBD_STATE_DEFAULT )
  {
    if ( p->wValue != 0 )
    {
      USBD_SetUsbState( USBD_STATE_ADDRESSED );
    }
    USBDHAL_SetAddr( p->wValue );
    retVal = USB_STATUS_OK;
  }

  else if ( pDev->state == USBD_STATE_ADDRESSED )
  {
    if ( p->wValue == 0 )
    {
      USBD_SetUsbState( USBD_STATE_DEFAULT );
    }
    USBDHAL_SetAddr( p->wValue );
    retVal = USB_STATUS_OK;
  }

  return retVal;
}

static USB_Status_TypeDef SetConfiguration( USBD_Device_TypeDef *pDev )
{
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( ( p->wIndex      != 0                          ) ||
       ( p->wLength     != 0                          ) ||
       ( (p->wValue>>8) != 0                          ) ||
       ( p->Recipient   != USB_SETUP_RECIPIENT_DEVICE )    )
  {
    return USB_STATUS_REQ_ERR;
  }

  if ( pDev->state == USBD_STATE_ADDRESSED )
  {
    if ( ( p->wValue == 0                                           ) ||
         ( p->wValue == pDev->configDescriptor->bConfigurationValue )    )
    {
      pDev->configurationValue = p->wValue;
      if ( p->wValue == pDev->configDescriptor->bConfigurationValue )
      {
        USBD_ActivateAllEps( true );
        USBD_SetUsbState( USBD_STATE_CONFIGURED );
      }
      retVal = USB_STATUS_OK;
    }
  }

  else if ( pDev->state == USBD_STATE_CONFIGURED )
  {
    if ( ( p->wValue == 0                                           ) ||
         ( p->wValue == pDev->configDescriptor->bConfigurationValue )    )
    {
      pDev->configurationValue = p->wValue;
      if ( p->wValue == 0 )
      {
        USBD_SetUsbState( USBD_STATE_ADDRESSED );
        USBD_DeactivateAllEps( USB_STATUS_DEVICE_UNCONFIGURED );
      }
      else
      {
        /* Reenable device endpoints, will reset data toggles */
        USBD_ActivateAllEps( false );
      }
      retVal = USB_STATUS_OK;
    }
  }

  return retVal;
}

static USB_Status_TypeDef SetFeature( USBD_Device_TypeDef *pDev )
{
  USBD_Ep_TypeDef *ep;
  USB_XferCompleteCb_TypeDef callback;
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  if ( p->wLength != 0 )
  {
    return USB_STATUS_REQ_ERR;
  }

  switch ( p->Recipient )
  {
    case USB_SETUP_RECIPIENT_DEVICE:
      if ( ( p->wIndex == 0                                ) &&  /* ITF no. 0 */
           ( p->wValue == USB_FEATURE_DEVICE_REMOTE_WAKEUP ) &&
           ( pDev->state == USBD_STATE_CONFIGURED          )    )
      {
        /* Remote wakeup feature set */
        if ( pDev->configDescriptor->bmAttributes & CONFIG_DESC_BM_REMOTEWAKEUP )
          {
          /* The device is capable of signalling remote wakeup */
          pDev->remoteWakeupEnabled = true;
          retVal = USB_STATUS_OK;
          }
      }
      break;

    case USB_SETUP_RECIPIENT_ENDPOINT:
      ep = USBD_GetEpFromAddr( p->wIndex & 0xFF );
      if ( ep )
      {
        if ( ( ep->num > 0                            ) &&
             ( p->wValue == USB_FEATURE_ENDPOINT_HALT ) &&
             ( pDev->state == USBD_STATE_CONFIGURED   )    )
        {
          retVal = USBDHAL_StallEp( ep );

          ep->state = D_EP_IDLE;
          /* Call end of transfer callback for endpoint */
          if ( ( retVal == USB_STATUS_OK ) &&
               ( ep->state != D_EP_IDLE  ) &&
               ( ep->xferCompleteCb      )    )
          {
            callback = ep->xferCompleteCb;
            ep->xferCompleteCb = NULL;
            DEBUG_USB_API_PUTS( "\nEP cb(), EP stalled" );
            callback( USB_STATUS_EP_STALLED, ep->xferred, ep->remaining);
          }
        }
      }
  }

  return retVal;
}

static USB_Status_TypeDef SetInterface( USBD_Device_TypeDef *pDev )
{
  USB_Setup_TypeDef *p = pDev->setup;
  USB_Status_TypeDef retVal = USB_STATUS_REQ_ERR;

  /* There is currently support for one interface and no alternate settings */

  if ( ( p->wIndex    == 0                             ) &&
       ( p->wLength   == 0                             ) &&
       ( p->wValue    == 0                             ) &&
       ( pDev->state  == USBD_STATE_CONFIGURED         ) &&
       ( p->Recipient == USB_SETUP_RECIPIENT_INTERFACE )    )
  {
    /* Reset data toggles on EP's */
    USBD_ActivateAllEps( false );
    return USB_STATUS_OK;
  }
  return retVal;
}

/** @endcond */

#endif /* defined( USB_DEVICE ) */
#endif /* defined( USB_PRESENT ) && ( USB_COUNT == 1 ) */
