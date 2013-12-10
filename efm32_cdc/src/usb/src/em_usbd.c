/**************************************************************************//**
 * @file
 * @brief USB protocol stack library, device API.
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

#include "em_cmu.h"
#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbd.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static USBD_Device_TypeDef device;
USBD_Device_TypeDef *dev = &device;

static const char *stateNames[] =
{
  [ USBD_STATE_NONE       ] = "NONE      ",
  [ USBD_STATE_ATTACHED   ] = "ATTACHED  ",
  [ USBD_STATE_POWERED    ] = "POWERED   ",
  [ USBD_STATE_DEFAULT    ] = "DEFAULT   ",
  [ USBD_STATE_ADDRESSED  ] = "ADDRESSED ",
  [ USBD_STATE_CONFIGURED ] = "CONFIGURED",
  [ USBD_STATE_SUSPENDED  ] = "SUSPENDED ",
  [ USBD_STATE_LASTMARKER ] = "UNDEFINED "
};

/** @endcond */

/***************************************************************************//**
 * @brief
 *   Abort all pending transfers.
 *
 * @details
 *   Aborts transfers for all endpoints currently in use. Pending
 *   transfers on the default endpoint (EP0) are not aborted.
 ******************************************************************************/
void USBD_AbortAllTransfers( void )
{
  INT_Disable();
  USBDHAL_AbortAllTransfers( USB_STATUS_EP_ABORTED );
  INT_Enable();
}

/***************************************************************************//**
 * @brief
 *   Abort a pending transfer on a specific endpoint.
 *
 * @param[in] epAddr
 *   The address of the endpoint to abort.
 ******************************************************************************/
int USBD_AbortTransfer( int epAddr )
{
  USB_XferCompleteCb_TypeDef callback;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_AbortTransfer(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_AbortTransfer(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();
  if ( ep->state == D_EP_IDLE )
  {
    INT_Enable();
    return USB_STATUS_OK;
  }

  USBD_AbortEp( ep );

  ep->state = D_EP_IDLE;
  if ( ep->xferCompleteCb )
  {
    callback = ep->xferCompleteCb;
    ep->xferCompleteCb = NULL;

    if ( ( dev->lastState == USBD_STATE_CONFIGURED ) &&
         ( dev->state     == USBD_STATE_ADDRESSED  )    )
    {
      USBDHAL_DeactivateEp( ep );
    }

    DEBUG_TRACE_ABORT( USB_STATUS_EP_ABORTED );
    callback( USB_STATUS_EP_ABORTED, ep->xferred, ep->remaining );
  }

  INT_Enable();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Start USB device operation.
 *
 * @details
 *   Device operation is started by connecting a pullup resistor on the
 *   appropriate USB data line.
 ******************************************************************************/
void USBD_Connect( void )
{
  INT_Disable();
  USBDHAL_Connect();
  INT_Enable();
}

/***************************************************************************//**
 * @brief
 *   Stop USB device operation.
 *
 * @details
 *   Device operation is stopped by disconnecting the pullup resistor from the
 *   appropriate USB data line. Often referred to as a "soft" disconnect.
 ******************************************************************************/
void USBD_Disconnect( void )
{
  INT_Disable();
  USBDHAL_Disconnect();
  INT_Enable();
}

/***************************************************************************//**
 * @brief
 *   Check if an endpoint is busy doing a transfer.
 *
 * @param[in] epAddr
 *   The address of the endpoint to check.
 *
 * @return
 *   True if endpoint is busy, false otherwise.
 ******************************************************************************/
bool USBD_EpIsBusy( int epAddr )
{
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_EpIsBusy(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->state == D_EP_IDLE )
    return false;

  return true;
}

/***************************************************************************//**
 * @brief
 *   Get current USB device state.
 *
 * @return
 *   Device USB state. See @ref USBD_State_TypeDef.
 ******************************************************************************/
USBD_State_TypeDef USBD_GetUsbState( void )
{
  return dev->state;
}

/***************************************************************************//**
 * @brief
 *   Get a string naming a device USB state.
 *
 * @param[in] state
 *   Device USB state. See @ref USBD_State_TypeDef.
 *
 * @return
 *   State name string pointer.
 ******************************************************************************/
const char *USBD_GetUsbStateName( USBD_State_TypeDef state )
{
  if ( state > USBD_STATE_LASTMARKER )
    state = USBD_STATE_LASTMARKER;

  return stateNames[ state ];
}

/***************************************************************************//**
 * @brief
 *   Initializes USB device hardware and internal protocol stack data structures,
 *   then connects the data-line (D+ or D-) pullup resistor to signal host that
 *   enumeration can begin.
 *
 * @note
 *   You may later use @ref USBD_Disconnect() and @ref USBD_Connect() to force
 *   reenumeration.
 *
 * @param[in] p
 *   Pointer to device initialization struct. See @ref USBD_Init_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_Init( const USBD_Init_TypeDef *p )
{
  int numEps;
  USBD_Ep_TypeDef *ep;
  uint8_t txFifoNum;
  uint8_t *conf, *confEnd;
  USB_EndpointDescriptor_TypeDef *epd;
  uint32_t totalRxFifoSize, totalTxFifoSize, numInEps, numOutEps;

  CMU_ClockSelectSet( cmuClock_HF, cmuSelect_HFXO );
#if ( USB_USBC_32kHz_CLK == USB_USBC_32kHz_CLK_LFXO )
  CMU_OscillatorEnable( cmuOsc_LFXO, true, false );
#else
  CMU_OscillatorEnable( cmuOsc_LFRCO, true, false );
#endif
  USBTIMER_Init();

  memset( dev, 0, sizeof( USBD_Device_TypeDef ) );

  dev->setup                = dev->setupPkt;
  dev->deviceDescriptor     = p->deviceDescriptor;
  dev->configDescriptor     = (USB_ConfigurationDescriptor_TypeDef*)
                              p->configDescriptor;
  dev->stringDescriptors    = p->stringDescriptors;
  dev->numberOfStrings      = p->numberOfStrings;
  dev->state                = USBD_STATE_LASTMARKER;
  dev->savedState           = USBD_STATE_NONE;
  dev->lastState            = USBD_STATE_NONE;
  dev->callbacks            = p->callbacks;
  dev->remoteWakeupEnabled  = false;

  /* Initialize EP0 */

  ep                 = &dev->ep[ 0 ];
  ep->in             = false;
  ep->buf            = NULL;
  ep->num            = 0;
  ep->mask           = 1;
  ep->addr           = 0;
  ep->type           = USB_EPTYPE_CTRL;
  ep->txFifoNum      = 0;
  ep->packetSize     = USB_EP0_SIZE;
  ep->remaining      = 0;
  ep->xferred        = 0;
  ep->state          = D_EP_IDLE;
  ep->xferCompleteCb = NULL;
  ep->fifoSize       = USB_EP0_SIZE / 4;

  totalTxFifoSize = ep->fifoSize * p->bufferingMultiplier[ 0 ];
  totalRxFifoSize = (ep->fifoSize + 1) * p->bufferingMultiplier[ 0 ];

#if defined( DEBUG_USB_API )
  /* Do a sanity check on the configuration descriptor */
  {
    int i;

    /* Check if bLength's adds up exactly to wTotalLength */
    i = 0;
    conf = (uint8_t*)dev->configDescriptor;
    confEnd = conf + dev->configDescriptor->wTotalLength;

    while ( conf < confEnd )
    {
      if ( *conf == 0 )
        break;

      i += *conf;
      conf += *conf;
    }

    if ( ( conf != confEnd                          ) ||
         ( i != dev->configDescriptor->wTotalLength )    )
    {
      DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal configuration descriptor" );
      EFM_ASSERT( false );
      return USB_STATUS_ILLEGAL;
    }
  }
#endif /* defined( DEBUG_USB_API ) */

  /* Parse configuration decriptor */
  numEps = 0;
  numInEps  = 0;
  numOutEps = 0;
  conf = (uint8_t*)dev->configDescriptor;
  confEnd = conf + dev->configDescriptor->wTotalLength;

  txFifoNum = 1;
  while ( conf < confEnd )
  {
    if ( *conf == 0 )
    {
      DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal configuration descriptor" );
      EFM_ASSERT( false );
      return USB_STATUS_ILLEGAL;
    }

    if ( *(conf + 1) == USB_ENDPOINT_DESCRIPTOR )
    {
      numEps++;
      epd = (USB_EndpointDescriptor_TypeDef*)conf;

      ep                 = &dev->ep[ numEps ];
      ep->in             = ( epd->bEndpointAddress & USB_SETUP_DIR_MASK ) != 0;
      ep->buf            = NULL;
      ep->addr           = epd->bEndpointAddress;
      ep->num            = ep->addr & USB_EPNUM_MASK;
      ep->mask           = 1 << ep->num;
      ep->type           = epd->bmAttributes & CONFIG_DESC_BM_TRANSFERTYPE;
      ep->packetSize     = epd->wMaxPacketSize;
      ep->remaining      = 0;
      ep->xferred        = 0;
      ep->state          = D_EP_IDLE;
      ep->xferCompleteCb = NULL;
      if ( ep->in )
      {
        numInEps++;
        ep->txFifoNum = txFifoNum++;
        ep->fifoSize = (ep->packetSize/4) * p->bufferingMultiplier[ numEps ];
        dev->inEpAddr2EpIndex[ ep->num ] = numEps;
        totalTxFifoSize += ep->fifoSize;
        if ( ep->num > MAX_NUM_IN_EPS )
        {
          DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal IN EP address" );
          EFM_ASSERT( false );
          return USB_STATUS_ILLEGAL;
        }
      }
      else
      {
        numOutEps++;
        ep->fifoSize = (ep->packetSize/4 + 1) * p->bufferingMultiplier[ numEps ];
        dev->outEpAddr2EpIndex[ ep->num ] = numEps;
        totalRxFifoSize += ep->fifoSize;
        if ( ep->num > MAX_NUM_OUT_EPS )
        {
          DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal OUT EP address" );
          EFM_ASSERT( false );
          return USB_STATUS_ILLEGAL;
        }
      }
    }
    conf += *conf;
  }

  /* Rx-FIFO size: SETUP packets : 4*n + 6    n=#CTRL EP's
   *               GOTNAK        : 1
   *               Status info   : 2*n        n=#OUT EP's (EP0 included) in HW
   */
  totalRxFifoSize += 10 + 1 + ( 2 * (MAX_NUM_OUT_EPS + 1) );

  if ( numEps != NUM_EP_USED )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal EP count" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( numInEps > MAX_NUM_IN_EPS )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal IN EP count" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( numOutEps > MAX_NUM_OUT_EPS )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Init(), Illegal OUT EP count" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();

  /* Enable USB clock */
  CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_USB | CMU_HFCORECLKEN0_USBC;
  CMU_ClockSelectSet( cmuClock_USBC, cmuSelect_HFCLK );

  USBHAL_DisableGlobalInt();

  if ( USBDHAL_CoreInit( totalRxFifoSize, totalTxFifoSize ) == USB_STATUS_OK )
  {
    USBDHAL_EnableUsbResetInt();
    USBHAL_EnableGlobalInt();
    NVIC_ClearPendingIRQ( USB_IRQn );
    NVIC_EnableIRQ( USB_IRQn );
  }
  else
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Init(), FIFO setup error" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

#if ( USB_PWRSAVE_MODE & USB_PWRSAVE_MODE_ONVBUSOFF )
  if ( USBHAL_VbusIsOn() )
  {
    USBD_SetUsbState( USBD_STATE_POWERED );
  }
  else
#endif
  {
    USBD_SetUsbState( USBD_STATE_NONE );
  }

  INT_Enable();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Start a read (OUT) transfer on an endpoint.
 *
 * @note
 *   The transfer buffer length must be a multiple of 4 bytes in length and
 *   WORD (4 byte) aligned. When allocating the buffer, round buffer length up.
 *   If it is possible that the host will send more data than your device
 *   expects, round buffer size up to the next multiple of maxpacket size.
 *
 * @param[in] epAddr
 *   Endpoint address.
 *
 * @param[in] data
 *   Pointer to transfer data buffer.
 *
 * @param[in] byteCount
 *   Transfer length.
 *
 * @param[in] callback
 *   Function to be called on transfer completion. Supply NULL if no callback
 *   is needed. See @ref USB_XferCompleteCb_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_Read( int epAddr, void *data, int byteCount,
               USB_XferCompleteCb_TypeDef callback )
{
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( (   byteCount > MAX_XFER_LEN                           ) ||
       ( ( byteCount / ep->packetSize ) > MAX_PACKETS_PR_XFER )    )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Illegal transfer size" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( (uint32_t)data & 3 )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Misaligned data buffer" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();
  if ( USBDHAL_EpIsStalled( ep ) )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Endpoint is halted" );
    return USB_STATUS_EP_STALLED;
  }

  if ( ep->state != D_EP_IDLE )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Endpoint is busy" );
    return USB_STATUS_EP_BUSY;
  }

  if ( ( ep->num > 0 ) && ( USBD_GetUsbState() != USBD_STATE_CONFIGURED ) )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Device not configured" );
    return USB_STATUS_DEVICE_UNCONFIGURED;
  }

  ep->buf       = (uint8_t*)data;
  ep->remaining = byteCount;
  ep->xferred   = 0;

  if ( ep->num == 0 )
  {
    ep->in = false;
  }
  else if ( ep->in != false )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Read(), Illegal EP direction" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  ep->state          = D_EP_RECEIVING;
  ep->xferCompleteCb = callback;

  USBD_ArmEp( ep );
  INT_Enable();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Perform a remote wakeup signalling sequence.
 *
 * @note
 *   It is the responsibility of the application to ensure that remote wakeup
 *   is not attempted before the device has been suspended for at least 5
 *   miliseconds. This function should not be called from within an interrupt
 *   handler.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_RemoteWakeup( void )
{
  INT_Disable();

  if ( ( dev->state != USBD_STATE_SUSPENDED ) ||
       ( dev->remoteWakeupEnabled == false  )    )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_RemoteWakeup(), Illegal remote wakeup" );
    return USB_STATUS_ILLEGAL;
  }

  USBDHAL_SetRemoteWakeup();
  INT_Enable();
  USBTIMER_DelayMs( 10 );
  INT_Disable();
  USBDHAL_ClearRemoteWakeup();
  INT_Enable();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Check if it is ok to enter energy mode EM2.
 *
 * @note
 *   Before entering EM2 both the USB hardware and the USB stack must be in a
 *   certain state, this function checks if all conditions for entering EM2
 *   is met.
 *   Refer to the @ref usb_device_powersave section for more information.
 *
 * @return
 *   True if ok to enter EM2, false otherwise.
 ******************************************************************************/
bool USBD_SafeToEnterEM2( void )
{
#if ( USB_PWRSAVE_MODE )
  return USBD_poweredDown ? true : false;
#else
  return false;
#endif
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

void USBD_SetUsbState( USBD_State_TypeDef newState )
{
  USBD_State_TypeDef currentState;

  currentState = dev->state;
  if ( newState == USBD_STATE_SUSPENDED )
  {
    dev->savedState = currentState;
  }

  dev->lastState = dev->state;
  dev->state = newState;

  if ( ( dev->callbacks->usbStateChange ) &&
       ( currentState != newState       )    )
  {
    dev->callbacks->usbStateChange( currentState, newState );
  }
}

/** @endcond */

/***************************************************************************//**
 * @brief
 *   Set an endpoint in the stalled (halted) state.
 *
 * @param[in] epAddr
 *   The address of the endpoint to stall.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_StallEp( int epAddr )
{
  USB_Status_TypeDef retVal;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_StallEp(), Illegal request" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_StallEp(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();
  retVal = USBDHAL_StallEp( ep );
  INT_Enable();

  if ( retVal != USB_STATUS_OK )
  {
    retVal = USB_STATUS_ILLEGAL;
  }

  return retVal;
}

/***************************************************************************//**
 * @brief
 *   Stop USB device stack operation.
 *
 * @details
 *   The data-line pullup resistor is turned off, USB interrupts are disabled,
 *   and finally the USB pins are disabled.
 ******************************************************************************/
void USBD_Stop( void )
{
  USBD_Disconnect();
  NVIC_DisableIRQ( USB_IRQn );
  USBHAL_DisableGlobalInt();
  USB->IEN   = _USB_IEN_RESETVALUE;
  USB->ROUTE = _USB_ROUTE_RESETVALUE;
  USBD_SetUsbState( USBD_STATE_NONE );
}

/***************************************************************************//**
 * @brief
 *   Reset stall state on a stalled (halted) endpoint.
 *
 * @param[in] epAddr
 *   The address of the endpoint to un-stall.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_UnStallEp( int epAddr )
{
  USB_Status_TypeDef retVal;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_UnStallEp(), Illegal request" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_UnStallEp(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();
  retVal = USBDHAL_UnStallEp( ep );
  INT_Enable();

  if ( retVal != USB_STATUS_OK )
  {
    retVal = USB_STATUS_ILLEGAL;
  }

  return retVal;
}

/***************************************************************************//**
 * @brief
 *   Start a write (IN) transfer on an endpoint.
 *
 * @param[in] epAddr
 *   Endpoint address.
 *
 * @param[in] data
 *   Pointer to transfer data buffer. This buffer must be WORD (4 byte) aligned.
 *
 * @param[in] byteCount
 *   Transfer length.
 *
 * @param[in] callback
 *   Function to be called on transfer completion. Supply NULL if no callback
 *   is needed. See @ref USB_XferCompleteCb_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code.
 ******************************************************************************/
int USBD_Write( int epAddr, void *data, int byteCount,
                USB_XferCompleteCb_TypeDef callback )
{
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr( epAddr );

  if ( ep == NULL )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Illegal endpoint" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( (   byteCount > MAX_XFER_LEN                           ) ||
       ( ( byteCount / ep->packetSize ) > MAX_PACKETS_PR_XFER )    )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Illegal transfer size" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  if ( (uint32_t)data & 3 )
  {
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Misaligned data buffer" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  INT_Disable();
  if ( USBDHAL_EpIsStalled( ep ) )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Endpoint is halted" );
    return USB_STATUS_EP_STALLED;
  }

  if ( ep->state != D_EP_IDLE )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Endpoint is busy" );
    return USB_STATUS_EP_BUSY;
  }

  if ( ( ep->num > 0 ) && ( USBD_GetUsbState() != USBD_STATE_CONFIGURED ) )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Device not configured" );
    return USB_STATUS_DEVICE_UNCONFIGURED;
  }

  ep->buf       = (uint8_t*)data;
  ep->remaining = byteCount;
  ep->xferred   = 0;

  if ( ep->num == 0 )
  {
    ep->in = true;
  }
  else if ( ep->in != true )
  {
    INT_Enable();
    DEBUG_USB_API_PUTS( "\nUSBD_Write(), Illegal EP direction" );
    EFM_ASSERT( false );
    return USB_STATUS_ILLEGAL;
  }

  ep->state          = D_EP_TRANSMITTING;
  ep->xferCompleteCb = callback;

  USBD_ArmEp( ep );
  INT_Enable();
  return USB_STATUS_OK;
}

/******** THE REST OF THE FILE IS DOCUMENTATION ONLY !**********************//**
 * @{

@page usb_device USB device stack library

  The source files for the USB device stack resides in the usb directory
  and follows the naming convention: em_usbd<em>nnn</em>.c/h.

  @li @ref usb_device_intro
  @li @ref usb_device_api
  @li @ref usb_device_conf
  @li @ref usb_device_powersave
  @li @ref usb_device_example1


@n @section usb_device_intro Introduction

  The USB device protocol stack provides an API which makes it possible to
  create USB devices with a minimum of effort. The device stack supports control,
  bulk and interrupt transfers.

  The stack is highly configurable to suit various needs, it does also contain
  useful debugging features together with several demonstration projects to
  get you started fast.

  We recommend that you read through this documentation, then proceed to build
  and test a few example projects before you start designing your own device.

@n @section usb_device_api The device stack API

  This section contains brief descriptions of the functions in the API. You will
  find detailed information on input and output parameters and return values by
  clicking on the hyperlinked function names. It is also a good idea to study
  the code in the USB demonstration projects.

  Your application code must include one header file: @em em_usb.h.

  All functions defined in the API can be called from within interrupt handlers.

  The USB stack use a hardware timer to keep track of time. TIMER0 is the
  default choice, refer to @ref usb_device_conf for other possibilities.
  Your application must not use the selected timer.

  <b>Pitfalls:</b>@n
    The USB peripheral will fill your receive buffers in quantities of WORD's
    (4 bytes). Transmit and receive buffers must be WORD aligned, in
    addition when allocating storage for receive buffers, round size up to
    next WORD boundary. If it is possible that the host will send more data
    than your device expects, round buffer size up to the next multiple of
    maxpacket size for the relevant endpoint to avoid data corruption.

    Transmit buffers passed to @htmlonly USBD_Write() @endhtmlonly must be
    statically allocated because @htmlonly USBD_Write() @endhtmlonly only
    initiates the transfer. When the host decide to actually perform the
    transfer, your data must be available.

  @n @ref USBD_Init() @n
    This function is called to register your device and all its properties with
    the device stack. The application must fill in a @ref USBD_Init_TypeDef
    structure prior to calling. Refer to @ref DeviceInitCallbacks for the
    optional callback functions defined within this structure. When this
    function has been called your device is ready to be enumerated by the USB
    host.

  @ref USBD_Read(), @ref USBD_Write() @n
    These functions initiate data transfers.
    @n @htmlonly USBD_Read() @endhtmlonly initiate a transfer of data @em
    from host @em to device (an @em OUT transfer in USB terminology).
    @n @htmlonly USBD_Write() @endhtmlonly initiate a transfer of data @em from
    device @em to host (an @em IN transfer).

    When the USB host actually performs the transfer, your application will be
    notified by means of a callback function which you provide (optionally).
    Refer to @ref TransferCallback for details of the callback functionality.

  @ref USBD_AbortTransfer(), @ref USBD_AbortAllTransfers() @n
    These functions terminate transfers that are initiated, but has not yet
    taken place. If a transfer is initiated with @htmlonly USBD_Read()
    or USBD_Write(), @endhtmlonly but the USB host never actually peform
    the transfers, these functions will deactivate the transfer setup to make
    the USB device endpoint hardware ready for new (and potentially) different
    transfers.

  @ref USBD_Connect(), @ref USBD_Disconnect() @n
    These functions turns the data-line (D+ or D-) pullup on or off. They can be
    used to force reenumeration. It's good practice to delay at least one second
    between @htmlonly USBD_Disconnect() and USBD_Connect() @endhtmlonly
    to allow the USB host to unload the currently active device driver.

  @ref USBD_EpIsBusy() @n
    Check if an endpoint is busy.

  @ref USBD_StallEp(), @ref USBD_UnStallEp() @n
    These functions stalls or un-stalls an endpoint. This functionality may not
    be needed by your application, but the USB device stack use them in response
    to standard setup commands SET_FEATURE and CLEAR_FEATURE. They may be useful
    when implementing some USB classes, e.g. a mass storage device use them
    extensively.

  @ref USBD_RemoteWakeup() @n
    Used in SUSPENDED state (see @ref USB_Status_TypeDef) to signal resume to
    host. It's the applications responsibility to adhere to the USB standard
    which states that a device can not signal resume before it has been
    SUSPENDED for at least 5 ms. The function will also check the configuration
    descriptor defined by the application to see if it is legal for the device
    to signal resume.

  @ref USBD_GetUsbState() @n
    Returns the device USB state (see @ref USBD_State_TypeDef). Refer to
    Figure 9-1. "Device State Diagram" in the USB revision 2.0 specification.

  @ref USBD_GetUsbStateName() @n
    Returns a text string naming a given USB device state.

  @ref USBD_SafeToEnterEM2() @n
    Check if it is ok to enter energy mode EM2. Refer to the
    @ref usb_device_powersave section for more information.

  @n @anchor TransferCallback <b>The transfer complete callback function:</b> @n
    @n USB_XferCompleteCb_TypeDef() is called when a transfer completes. It is
    called with three parameters, the status of the transfer, the number of
    bytes transferred and the number of bytes remaining. It may not always be
    needed to have a callback on transfer completion, but you should keep in
    mind that a transfer may be aborted when you least expect it. A transfer
    will be aborted if host stalls the endpoint, if host resets your device, if
    host unconfigures your device or if you unplug your device cable and the
    device is selfpowered.
    @htmlonly USB_XferCompleteCb_TypeDef() @endhtmlonly is also called if your
    application use @htmlonly USBD_AbortTransfer() or USBD_AbortAllTransfers()
    @endhtmlonly calls.
    @note This callback is called from within an interrupt handler with
          interrupts disabled.

  @n @anchor DeviceInitCallbacks <b>Optional callbacks passed to the stack via
    the @ref USBD_Init() function:</b> @n
    @n These callbacks are all optional, and it is up to the application
    programmer to decide if the application needs the functionality they
    provide.
    @note These callbacks are all called from within an interrupt handler
          with interrupts disabled.

  USBD_UsbResetCb_TypeDef() is called each time reset signalling is sensed on
    the USB wire.

  @n USBD_SofIntCb_TypeDef() is called with framenumber as a parameter on
    each SOF interrupt.

  @n USBD_DeviceStateChangeCb_TypeDef() is called whenever the device state
    change. Useful for detecting e.g. SUSPENDED state change in order to reduce
    current consumption of buspowered devices. The USB HID keyboard example
    project has a good example on how to use this callback.

  @n USBD_IsSelfPoweredCb_TypeDef() is called by the device stack when host
    queries the device with a standard setup GET_STATUS command to check if the
    device is currently selfpowered or buspowered. This feature is only
    applicable on selfpowered devices which also works when only buspower is
    available.

  @n USBD_SetupCmdCb_TypeDef() is called each time a setup command is
    received from host. Use this callback to override or extend the default
    handling of standard setup commands, and to implement class or vendor
    specific setup commands. The USB HID keyboard example project has a good
    example on how to use this callback.

  @n <b>Utility functions:</b> @n
    @n    USB_PUTCHAR() Transmit a single char on the debug serial port.
    @n @n USB_PUTS() Transmit a zero terminated string on the debug serial port.
    @n @n USB_PRINTF() Transmit "printf" formated data on the debug serial port.
    @n @n USB_GetErrorMsgString() Return an error message string for a given
          error code.
    @n @n USB_PrintErrorMsgString() Format and print a text string given an
          error code, prepends an optional user supplied leader string.
    @n @n USBTIMER_DelayMs() Active wait millisecond delay function. Can also be
          used inside interrupt handlers.
    @n @n USBTIMER_DelayUs() Active wait microsecond delay function. Can also be
          used inside interrupt handlers.
    @n @n USBTIMER_Init() Initialize the timer system. Called by @htmlonly
          USBD_Init(), @endhtmlonly but your application must call it again to
          reinitialize whenever you change the HFPERCLK frequency.
    @n @n USBTIMER_Start() Start a timer. You can configure the USB device stack
          to provide any number of timers. The timers have 1 ms resolution, your
          application is notified of timeout by means of a callback.
    @n @n USBTIMER_Stop() Stop a timer.

@n @section usb_device_conf Configuring the device stack

  Your application must provide a header file named @em usbconfig.h. This file
  must contain the following \#define's:@n @n
  @verbatim
#define USB_DEVICE       // Compile the stack for device mode.
#define NUM_EP_USED n    // Your application use 'n' endpoints in
                         // addition to endpoint 0. @endverbatim

  @n @em usbconfig.h may define the following items: @n @n
  @verbatim
#define NUM_APP_TIMERS n // Your application needs 'n' timers

#define DEBUG_USB_API    // Turn on API debug diagnostics.

// Some utility functions in the API needs printf. These
// functions have "print" in their name. This macro enables
// these functions.
#define USB_USE_PRINTF   // Enable utility print functions.

// Define a function for transmitting a single char on the serial port.
extern int RETARGET_WriteChar(char c);
#define USER_PUTCHAR  RETARGET_WriteChar

#define USB_TIMER USB_TIMERn  // Select which hardware timer the USB stack
                              // is allowed to use. Valid values are n=0,1,2...
                              // corresponding to TIMER0, TIMER1, ...
                              // If not specified, TIMER0 is used @endverbatim

  @n You are strongly encouraged to start application development with DEBUG_USB_API
  turned on. When DEBUG_USB_API is turned on and USER_PUTCHAR is defined, useful
  debugging information will be output on the development kit serial port.
  Compiling with the DEBUG_EFM_USER flag will also enable all asserts
  in both @em emlib and in the USB stack. If asserts are enabled and
  USER_PUTCHAR defined, assert texts will be output on the serial port.

  You application must include @em retargetserial.c if DEBUG_USB_API is defined
  and @em retargetio.c if USB_USE_PRINTF is defined.
  These files reside in the @em drivers
  directory in the software package for your development board. Refer to
  @ref usb_device_powersave for energy-saving mode configurations.

@n @section usb_device_powersave Energy-saving modes

  The device stack provides two energy saving levels. The first level is to
  set the USB peripheral in energy saving mode, the next level is to enter
  Energy Mode 2 (EM2). These energy saving modes can be applied when the device
  is suspended by the USB host, or when when the device is not connected to a
  USB host.
  In addition to this an application can use energy modes EM1 and EM2. There
  are no restrictions on when EM1 can be entered, EM2 can only be entered
  when the USB device is suspended or detached from host.

  Energy-saving modes are selected with a \#define in @em usbconfig.h, default
  selection is to not use any energy saving modes.@n @n
  @verbatim
#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ENTEREM2)@endverbatim

  There are three flags available, the flags can be or'ed together as shown above.

  <b>\#define USB_PWRSAVE_MODE_ONSUSPEND</b>@n Set USB peripheral in low power
  mode on suspend.

  <b>\#define USB_PWRSAVE_MODE_ONVBUSOFF</b>@n Set USB peripheral in low power
  mode when not attached to a host. This mode assumes that the internal voltage
  regulator is used and that the VREGI pin of the chip is connected to VBUS.
  This option can not be used with bus-powered devices.

  <b>\#define USB_PWRSAVE_MODE_ENTEREM2</b>@n Enter EM2 when USB peripheral is
  in low power mode.

  When the USB peripheral is set in low power mode, it must be clocked by a 32kHz
  clock. Both LFXO and LFRCO can be used, but only LFXO guarantee USB specification
  compliance. Selection is done with a \#define in @em usbconfig.h.@n @n
  @verbatim
#define USB_USBC_32kHz_CLK   USB_USBC_32kHz_CLK_LFXO @endverbatim
  Two flags are available, <b>USB_USBC_32kHz_CLK_LFXO</b> and
  <b>USB_USBC_32kHz_CLK_LFRCO</b>. <b>USB_USBC_32kHz_CLK_LFXO</b> is selected
  by default.

  The USB HID keyboard and Mass Storage device example projects demonstrate
  different energy-saving modes.

  <b>Example 1:</b>
  Leave all energy saving to the stack, the device enters EM2 on suspend and
  when detached from host. @n
  @verbatim
In usbconfig.h:

#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ONVBUSOFF | USB_PWRSAVE_MODE_ENTEREM2)
  @endverbatim

  @n <b>Example 2:</b>
  Let the stack control energy saving in the USB periheral but let your
  application control energy modes EM1 and EM2. @n
  @verbatim
In usbconfig.h:

#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ONVBUSOFF)

In application code:

if ( USBD_SafeToEnterEM2() )
  EMU_EnterEM2(true);
else
  EMU_EnterEM1(); @endverbatim

@n @section usb_device_example1 Vendor unique device example application

  This example represents the most simple USB device imaginable. It's purpose
  is to turn user LED's on or off under control of vendor unique setup commands.
  The device will rely on @em libusb device driver on the host, a host
  application @em EFM32-LedApp.exe is bundled with the example.

  The main() is really simple ! @n @n
  @verbatim
#include "em_usb.h"

#include "descriptors.h"

int main( void )
{
  BSP_Init(BSP_INIT_DEFAULT); // Initialize DK board register access
  CMU_ClockSelectSet( cmuClock_HF, cmuSelect_HFXO );
  BSP_LedsSet(0);             // Turn off all LED's

  ConsoleDebugInit();         // Initialize UART for debug diagnostics

  USB_PUTS( "\nEFM32 USB LED Vendor Unique Device example\n" );

  USBD_Init( &initstruct );   // GO !

  //When using a debugger it is pratical to uncomment the following three
  //lines to force host to re-enumerate the device.

  //USBD_Disconnect();
  //USBTIMER_DelayMs( 1000 );
  //USBD_Connect();

  for (;;) {}
} @endverbatim

  @n Configure the device stack in <em>usbconfig.h</em>: @n @n
  @verbatim
#define USB_DEVICE                        // Compile stack for device mode.

// **************************************************************************
**                                                                         **
** Specify number of endpoints used (in addition to EP0).                  **
**                                                                         **
*****************************************************************************
#define NUM_EP_USED 0                     // EP0 is the only endpoint used.

// **************************************************************************
**                                                                         **
** Configure serial port debug output.                                     **
**                                                                         **
*****************************************************************************
// Prototype a function for transmitting a single char on the serial port.
extern int RETARGET_WriteChar(char c);
#define USER_PUTCHAR RETARGET_WriteChar

// Enable debug diagnostics from API functions (illegal input params etc.)
#define DEBUG_USB_API @endverbatim

  @n Define device properties and fill in USB initstruct in
  <em>descriptors.h</em>: @n @n
  @verbatim
EFM32_ALIGN(4)
static const USB_DeviceDescriptor_TypeDef deviceDesc __attribute__ ((aligned(4))) =
{
  .bLength            = USB_DEVICE_DESCSIZE,
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xFF,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_EP0_SIZE,
  .idVendor           = 0x2544,
  .idProduct          = 0x0001,
  .bcdDevice          = 0x0000,
  .iManufacturer      = 1,
  .iProduct           = 2,
  .iSerialNumber      = 3,
  .bNumConfigurations = 1
};

EFM32_ALIGN(4)
static const uint8_t configDesc[] __attribute__ ((aligned(4)))=
{
  // *** Configuration descriptor ***
  USB_CONFIG_DESCSIZE,            // bLength
  USB_CONFIG_DESCRIPTOR,          // bDescriptorType
  USB_CONFIG_DESCSIZE +           // wTotalLength (LSB)
  USB_INTERFACE_DESCSIZE,
  (USB_CONFIG_DESCSIZE +          // wTotalLength (MSB)
  USB_INTERFACE_DESCSIZE)>>8,
  1,                              // bNumInterfaces
  1,                              // bConfigurationValue
  0,                              // iConfiguration
  CONFIG_DESC_BM_RESERVED_D7 |    // bmAttrib: Self powered
  CONFIG_DESC_BM_SELFPOWERED,
  CONFIG_DESC_MAXPOWER_mA( 100 ), // bMaxPower: 100 mA

  // *** Interface descriptor ***
  USB_INTERFACE_DESCSIZE,         // bLength
  USB_INTERFACE_DESCRIPTOR,       // bDescriptorType
  0,                              // bInterfaceNumber
  0,                              // bAlternateSetting
  NUM_EP_USED,                    // bNumEndpoints
  0xFF,                           // bInterfaceClass
  0,                              // bInterfaceSubClass
  0,                              // bInterfaceProtocol
  0,                              // iInterface
};

STATIC_CONST_STRING_DESC_LANGID( langID, 0x04, 0x09 );
STATIC_CONST_STRING_DESC( iManufacturer, 'E','n','e','r','g','y',' ',       \
                                         'M','i','c','r','o',' ','A','S' );
STATIC_CONST_STRING_DESC( iProduct     , 'V','e','n','d','o','r',' ',       \
                                         'U','n','i','q','u','e',' ',       \
                                         'L','E','D',' ',                   \
                                         'D','e','v','i','c','e' );
STATIC_CONST_STRING_DESC( iSerialNumber, '0','0','0','0','0','0',           \
                                         '0','0','1','2','3','4' );

static const void * const strings[] =
{
  &langID,
  &iManufacturer,
  &iProduct,
  &iSerialNumber
};

// Endpoint buffer sizes
// 1 = single buffer, 2 = double buffering, 3 = tripple buffering ...
static const uint8_t bufferingMultiplier[ NUM_EP_USED + 1 ] = { 1 };

static const USBD_Callbacks_TypeDef callbacks =
{
  .usbReset        = NULL,
  .usbStateChange  = NULL,
  .setupCmd        = SetupCmd,
  .isSelfPowered   = NULL,
  .sofInt          = NULL
};

static const USBD_Init_TypeDef initstruct =
{
  .deviceDescriptor    = &deviceDesc,
  .configDescriptor    = configDesc,
  .stringDescriptors   = strings,
  .numberOfStrings     = sizeof(strings)/sizeof(void*),
  .callbacks           = &callbacks,
  .bufferingMultiplier = bufferingMultiplier
};  @endverbatim

  @n Now we have to implement vendor unique USB setup commands to control the
  LED's (see callbacks variable above). Notice that the buffer variable below is
  statically allocated because @htmlonly USBD_Write() @endhtmlonly only
  initiates the transfer. When the host actually performs the transfer, the
  SetupCmd() function will have returned ! @n @n

  @verbatim
#define VND_GET_LEDS 0x10
#define VND_SET_LED  0x11

static int SetupCmd( const USB_Setup_TypeDef *setup )
{
  int retVal;
  uint16_t leds;
  static uint32_t buffer;
  uint8_t *pBuffer = (uint8_t*)&buffer;

  retVal = USB_STATUS_REQ_UNHANDLED;

  if ( setup->Type == USB_SETUP_TYPE_VENDOR )
  {
    switch ( setup->bRequest )
    {
      case VND_GET_LEDS:
      // ********************
        *pBuffer = BSP_LedsGet() & 0x1F;
        retVal = USBD_Write( 0, pBuffer, setup->wLength, NULL );
        break;

      case VND_SET_LED:
      // ********************
        leds = DVK_getLEDs() & 0x1F;
        if ( setup->wValue )
        {
          leds |= LED0 << setup->wIndex;
        }
        else
        {
          leds &= ~( LED0 << setup->wIndex );
        }
        BSP_LedsSet( leds );
        retVal = USB_STATUS_OK;
        break;
    }
  }

  return retVal;
}@endverbatim

 * @}**************************************************************************/

#endif /* defined( USB_DEVICE ) */
#endif /* defined( USB_PRESENT ) && ( USB_COUNT == 1 ) */
