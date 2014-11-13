#ifndef  __CONFIG_H__

#include <uvisor-config.h>

/* ensure  that SoC initialization is ignored */
#define NOSYSTEM

#define CHANNEL_DEBUG 1
#define XTAL_FREQUENCY 48000000UL

#define USE_FLASH_SIZE UVISOR_FLASH_SIZE
#define USE_SRAM_SIZE  UVISOR_SRAM_SIZE
#define RESERVED_FLASH 0x420
#define RESERVED_SRAM  0x200

#define STACK_MINIMUM_SIZE 2048

#endif /*__CONFIG_H__*/
