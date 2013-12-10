/***************************************************************************//**
 * @file
 * @brief USB protocol stack library, USB host endpoint handlers.
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
#if defined( USB_PRESENT ) && ( USB_COUNT == 1 )
#include "em_usb.h"
#if defined( USB_HOST )

#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbh.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/*
 * USBHEP_CtrlEpHandler() is called each time a packet has been transmitted
 * or recieved on host channels serving a device default endpoint (EP0).
 * A state machine navigate us through the phases of a control transfer
 * according to "chapter 9" in the USB spec.
 */
void USBHEP_CtrlEpHandler( USBH_Ep_TypeDef *ep, USB_Status_TypeDef result )
{
uint8_t direction;

  switch ( ep->state )
  {
    /* ------- Setup stage ------------------------------------------------ */
    case H_EP_SETUP:                      /* SETUP packet sent successfully */
      if ( result == USB_STATUS_OK )
      {
        direction = ep->setup.bmRequestType & USB_SETUP_DIR_MASK;

        /* Check if there is a data stage */
        if ( ep->setup.wLength != 0 )
        {
          if ( direction == USB_SETUP_DIR_D2H )   /* Data direction is IN */
          {
            USBH_CtlReceiveData( ep, ep->setup.wLength );
            ep->state = H_EP_DATA_IN;
            break;
          }
          else                                    /* Data direction is OUT */
          {
            USBH_CtlSendData( ep, ep->setup.wLength );
            ep->state = H_EP_DATA_OUT;
            break;
          }
        }
        USBH_CtlReceiveData( ep, 0 );   /* No data stage */
        ep->state = H_EP_STATUS_IN;     /* Get zero length in status packet */
        break;
      }

      ep->setupErrCnt++;

      /*
       * After a halt condition is encountered or an error is detected by the
       * host, a control endpoint is allowed to recover by accepting the next
       * SETUP package. If the next Setup package is not accepted, a device
       * reset will ultimately be needed.
       */

      if ( ( ep->setupErrCnt == 1 ) && ( ep->timeout != 0 ) )
      {
        USBH_CtlSendSetup( ep );                  /* Retry */
        break;
      }
      USBHEP_TransferDone( ep, result );
      break;


    /* ------- Data IN stage ---------------------------------------------- */
    case H_EP_DATA_IN:                /* Data received */
      if ( result == USB_STATUS_OK )
      {
        USBH_CtlSendData( ep, 0 );    /* Send zero length out status packet */
        ep->state = H_EP_STATUS_OUT;
        break;
      }
      USBHEP_TransferDone( ep, result );
      break;


    /* ------- Data OUT stage --------------------------------------------- */
    case H_EP_DATA_OUT:                 /* Data sent */
      if ( result == USB_STATUS_OK )
      {
        USBH_CtlReceiveData( ep, 0 );   /* Get zero length in status packet */
        ep->state = H_EP_STATUS_IN;
        break;
      }
      USBHEP_TransferDone( ep, result );
      break;


    /* ------- Status IN/OUT stage ---------------------------------------- */
    case H_EP_STATUS_IN:                /* Status packet received */
    case H_EP_STATUS_OUT     :          /* Status packet sent */
      USBHEP_TransferDone( ep, result );
      break;


    case H_EP_IDLE:
      break;
  }
}

/*
 * USBHEP_EpHandler() is called each time a packet has been transmitted
 * or recieved on a host channel serving a non-control device endpoint.
 */
void USBHEP_EpHandler( USBH_Ep_TypeDef *ep, USB_Status_TypeDef result )
{
  switch ( ep->state )
  {
    case H_EP_DATA_IN:
    case H_EP_DATA_OUT:
      USBHEP_TransferDone( ep, result );
      break;

    default:
    case H_EP_IDLE:
      break;
  }
}

/*
 * USBHEP_TransferDone() is called on transfer completion on all types of
 * device endpoints.
 */
void USBHEP_TransferDone( USBH_Ep_TypeDef *ep, USB_Status_TypeDef result )
{
  int hcnum;
  USB_XferCompleteCb_TypeDef callback;

  ep->xferCompleted = true;
  ep->xferStatus = result;
  ep->state = H_EP_IDLE;

  if ( ep->type == USB_EPTYPE_CTRL )
  {
    hcnum = ep->setup.Direction == USB_SETUP_DIR_IN ? ep->hcIn : ep->hcOut;

    if ( ep->timeout )
      USBTIMER_Stop( ep->hcOut + HOSTCH_TIMER_INDEX );

    ep->xferred   = hcs[ hcnum ].xferred;
    ep->remaining = hcs[ hcnum ].remaining;

    hcs[ ep->hcIn ].idle = true;
    hcs[ ep->hcOut ].idle = true;

    if ( ep->setup.wLength == 0 )
    {
      ep->xferred = 0;
    }
  }
  else
  {
    hcnum = ep->in ? ep->hcIn : ep->hcOut;

    if ( ep->timeout || ep->type == USB_EPTYPE_INTR )
      USBTIMER_Stop( hcnum + HOSTCH_TIMER_INDEX );

    hcs[ hcnum ].idle = true;
    ep->xferred   = hcs[ hcnum ].xferred;
    ep->remaining = hcs[ hcnum ].remaining;
  }

  if ( ep->xferCompleteCb )
  {
    callback = ep->xferCompleteCb;
    ep->xferCompleteCb = NULL;
    (callback)( result, ep->xferred, ep->remaining );
  }
}

/** @endcond */

#endif /* defined( USB_HOST ) */
#endif /* defined( USB_PRESENT ) && ( USB_COUNT == 1 ) */
