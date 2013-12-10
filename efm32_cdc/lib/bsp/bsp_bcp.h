/**************************************************************************//**
 * @file
 * @brief Board Controller Communications Protocol (BCP) definitions
 * @author Energy Micro AS
 * @version 3.20.2
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
#ifndef __BSP_BCP_H
#define __BSP_BCP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup BSP_STK API for STK's
 * @{
 ******************************************************************************/

/* BCP Packet Types */
#define BSP_BCP_INVALID          0   /**< Invalid packet received */

#define BSP_BCP_FIRST            1   /**< Smallest numerical value of  message type */

#define BSP_BCP_ACK              5   /**< Generic ACK for one way packages */
#define BSP_BCP_ECHO_REQ         10  /**< EFM32 BC alive request */
#define BSP_BCP_ECHO_REPLY       11  /**< BC alive response */
#define BSP_BCP_CURRENT_REQ      14  /**< EFM32 Request AEM current */
#define BSP_BCP_CURRENT_REPLY    16  /**< BC Response AEM current */
#define BSP_BCP_VOLTAGE_REQ      18  /**< EFM32 Request AEM voltage */
#define BSP_BCP_VOLTAGE_REPLY    20  /**< BC Response AEM voltage */
#define BSP_BCP_ENERGYMODE       22  /**< EFM32 Report Energy Mode (for AEM) */
#define BSP_BCP_STDOUT           24  /**< Debug packet (not used)  */
#define BSP_BCP_STDERR           26  /**< Debug packet (not used)  */
#define BSP_BCP_TEST             32  /**< Reserved type for test */
#define BSP_BCP_TEST_REPLY       33  /**< Reserved type for test (reply) */

#define BSP_BCP_LAST             100 /**< Last defined message type */

#define BSP_BCP_MAGIC            0xF1 /**< Magic byte to indicate start of packet */




#if ( ( BSP_BCP_VERSION == 1 ) || DOXY_DOC_ONLY )

#ifdef DOXY_DOC_ONLY
                                    /* Hack for doxygen doc ! */
#define BSP_BCP_PACKET_SIZe      30 /**< Max packet size for version 1 of the protocol. */
#else
#define BSP_BCP_PACKET_SIZE      30 /**< Max packet size for version 1 of the protocol. */
#endif

/** @brief BCP Packet Structure - Board controller communication protocol version 1. */
typedef struct
{
  uint8_t magic;                      /**< Magic - start of packet - must be BSP_BCP_MAGIC */
  uint8_t type;                       /**< Type of packet */
  uint8_t payloadLength;              /**< Length of data segment >=0 and <=BSP_BCP_PACKET_SIZE */
  uint8_t data[BSP_BCP_PACKET_SIZE];  /**< BCP Packet Data payload */
#ifdef DOXY_DOC_ONLY
} BCP_Packet_;                        /* Hack for doxygen doc ! */
#else
} BCP_Packet;
#endif
#endif




#if ( ( BSP_BCP_VERSION == 2 ) || DOXY_DOC_ONLY )

#define BSP_BCP_PACKET_SIZE      132  /**< Max packet size for version 2 of the protocol. */

/** @brief BCP Packet Structure - Board controller communication protocol version 2. */
typedef struct
{
  uint8_t magic;                      /**< Magic - start of packet - must be BSP_BCP_MAGIC */
  uint8_t type;                       /**< Type - packet type */
  uint8_t payloadLength;              /**< Length of data segment >=0 and <=BSP_BCP_PACKET_SIZE */
  uint8_t reserved;                   /**< Reserved for future expansion */
  uint8_t data[BSP_BCP_PACKET_SIZE];  /**< BCP Packet Data payload */
} BCP_Packet;

/** @brief BCP Packet Header definition */
typedef struct
{
  uint8_t magic;                    /**< Magic - start of packet - must be BSP_BCP_MAGIC */
  uint8_t type;                     /**< Type - packet type */
  uint8_t payloadLength;            /**< Length of data segment >=0 and <=BSP_BCP_PACKET_SIZE */
  uint8_t reserved;                 /**< Reserved for future expansion */
} BCP_PacketHeader;
#endif




#if ( ( BSP_BCP_VERSION != 1 ) && ( BSP_BCP_VERSION != 2 ) )
#error "BSP BCP Board Controller communications protocol version error."
#endif

#if ( BSP_BCP_PACKET_SIZE >= 255 )
#error "BSP BCP Board Controller communications packets must be less than 255 bytes in size!"
#endif

/** @} (end group BSP_STK) */

#ifdef __cplusplus
}
#endif

#endif  /* __BSP_BCP_H */
