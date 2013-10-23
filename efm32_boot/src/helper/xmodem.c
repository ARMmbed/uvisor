/**************************************************************************//**
 * @file
 * @brief XMODEM protocol
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
#include "xmodem.h"
#include "usart.h"
#include "flash.h"
#include "crc.h"

#define ALIGNMENT(base,align) (((base)+((align)-1))&(~((align)-1)))
uint8_t rawPacket[2][ALIGNMENT(sizeof(XMODEM_packet),4)] __attribute__ ((aligned(4)));

/**************************************************************************//**
 * @brief Verifies checksum, packet numbering and
 * @param pkt The packet to verify
 * @param sequenceNumber The current sequence number.
 * @returns -1 on packet error, 0 otherwise
 *****************************************************************************/
__INLINE int XMODEM_verifyPacketChecksum(XMODEM_packet *pkt, int sequenceNumber)
{
  uint16_t packetCRC;
  uint16_t calculatedCRC;

  /* Check the packet number integrity */
  if (pkt->packetNumber + pkt->packetNumberC != 255)
  {
    return -1;
  }

  /* Check that the packet number matches the excpected number */
  if (pkt->packetNumber != (sequenceNumber % 256))
  {
    return -1;
  }

  calculatedCRC = CRC_calc((uint8_t *) pkt->data, (uint8_t *) &(pkt->crcHigh));
  packetCRC     = pkt->crcHigh << 8 | pkt->crcLow;

  /* Check the CRC value */
  if (calculatedCRC != packetCRC)
  {
    return -1;
  }
  return 0;
}

/**************************************************************************//**
 * @brief Starts a XMODEM download.
 *
 * @param baseAddress
 *   The address to start writing from
 *
 * @param endAddress
 *   The last address. This is only used for clearing the flash
 *****************************************************************************/
int XMODEM_download(uint32_t baseAddress, uint32_t endAddress)
{
  XMODEM_packet *pkt;
  uint32_t      i;
  uint32_t      addr;
  uint32_t      byte;
  uint32_t      sequenceNumber = 1;

  for (addr = baseAddress; addr < endAddress; addr += flashPageSize)
  {
    FLASH_eraseOneBlock(addr);
  }
  /* Send one start transmission packet. Wait for a response. If there is no
   * response, we resend the start transmission packet.
   * Note: This is a fairly long delay between retransmissions(~6 s). */
  while (1)
  {
    USART_txByte(XMODEM_NCG);
    for (i = 0; i < 10000000; i++)
    {
#ifdef BOOTLOADER_LEUART_CLOCK
      if (BOOTLOADER_USART->STATUS & LEUART_STATUS_RXDATAV)
#else
      if (BOOTLOADER_USART->STATUS & USART_STATUS_RXDATAV)
#endif
      {
        goto xmodem_transfer;
      }
    }
  }
 xmodem_transfer:
  while (1)
  {
    /* Swap buffer for packet buffer */
    pkt = (XMODEM_packet *) rawPacket[sequenceNumber & 1];

    /* Fetch the first byte of the packet explicitly, as it defines the
     * rest of the packet */
    pkt->header = USART_rxByte();

    /* Check for end of transfer */
    if (pkt->header == XMODEM_EOT)
    {
      /* Acknowledget End of transfer */
      USART_txByte(XMODEM_ACK);
      break;
    }

    /* If the header is not a start of header (SOH), then cancel *
     * the transfer. */
    if (pkt->header != XMODEM_SOH)
    {
      return -1;
    }

    /* Fill the remaining bytes packet */
    /* Byte 0 is padding, byte 1 is header */
    for (byte = 2; byte < sizeof(XMODEM_packet); byte++)
    {
      *(((uint8_t *) pkt) + byte) = USART_rxByte();
    }

    if (XMODEM_verifyPacketChecksum(pkt, sequenceNumber) != 0)
    {
      /* On a malformed packet, we send a NAK, and start over */
      USART_txByte(XMODEM_NAK);
      continue;
    }

    /* Write data to flash */
    FLASH_writeBlock((void *) baseAddress,
                     (sequenceNumber - 1) * XMODEM_DATA_SIZE,
                     XMODEM_DATA_SIZE,
                     (uint8_t const *) pkt->data);



    sequenceNumber++;
    /* Send ACK */
    USART_txByte(XMODEM_ACK);
  }
  /* Wait for the last DMA transfer to finish. */
  while (DMA->CHENS & DMA_CHENS_CH0ENS) ;
  return 0;
}
