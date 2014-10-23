#ifndef  __CONFIG_H__

#include <uvisor-config.h>

#define CHANNEL_DEBUG 0

#define XTAL_FREQUENCY 48000000UL
#define RESERVED_FLASH 2 * UVISOR_FLASH_SIZE
#define RESERVED_SRAM  2 * UVISOR_SRAM_SIZE

#endif /*__CONFIG_H__*/
