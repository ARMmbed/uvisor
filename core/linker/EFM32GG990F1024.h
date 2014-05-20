MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x100000  /*   1M */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x20000   /* 128k */
}

#include "EFM32.h"
