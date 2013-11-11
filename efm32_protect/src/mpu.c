#include <iot-os.h>
#include "mpu.h"

static const MPU_Region mpu_map[MPU_REGIONS] = {
    {MPU_RBAR(0,FLASH_MEM_BASE),MPU_RASR(SECURE_RAM_SIZE_BITS,)},
    {MPU_RBAR(1,RAM_CODE_MEM_BASE),},
    {MPU_RBAR(2,0x00000000),},
    {MPU_RBAR(3,0x00000000),}
};

void mpu_init(void)
{
//    dprintf("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
}
