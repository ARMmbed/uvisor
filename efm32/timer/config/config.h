#ifndef  __CONFIG_H__

#include <uvisor-config.h>

#define CHANNEL_DEBUG 2

#define XTAL_FREQUENCY 48000000UL
#define RESERVED_FLASH 3 * UVISOR_FLASH_SIZE
#define RESERVED_SRAM  3 * UVISOR_SRAM_SIZE

#endif /*__CONFIG_H__*/
