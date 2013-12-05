#include <iot-os.h>
#include "mpu.h"

static uint32_t mpu_aligment_mask;

/* calculate rounded section alignment */
uint8_t mpu_region_bits(uint32_t size)
{
    uint8_t bits=0;

    /* find highest bit */
    while(size)
    {
        size>>=1;
        bits++;
    }

    /* smallest region is 32 bytes */
    return (bits<=5)?5:bits;
}

void mpu_debug(void)
{
    uint8_t r;
    dprintf("MPU region dump:\r\n");
    for(r=0; r<MPU_REGIONS; r++)
    {
        MPU->RNR = r;
        dprintf("\tR:%i RBAR=0x%08X RASR=0x%08X\r\n", r, MPU->RBAR, MPU->RASR);
    }
    dprintf("\tCTRL=0x%08X\r\n", MPU->CTRL);
}

/* returns rounded MPU section alignment bit count */
int mpu_set(uint8_t region, void* base, uint32_t size, uint32_t flags)
{
    uint8_t bits;
    uint32_t mask;

    /* get region bits */
    if(!(bits = mpu_region_bits(size)))
        return E_PARAM;

    bits--;

    /* round up mask */
    mask = (1<<bits)-1;
    /* extend mask if needed */
    if(mask & size)
    {
        mask = (mask<<1)+1;
        bits++;
    }

    /* verify alignment */
    if(((uint32_t)base) & mask)
        return E_ALIGN;

    MPU->RBAR = MPU_RBAR(region,(uint32_t)base);
    MPU->RASR =
        MPU_RASR_ENABLE_Msk |
        (bits<<MPU_RASR_SIZE_Pos) |
        flags;

    return 0;
}

void mpu_init(void)
{
    /* reset MPU */
    MPU->CTRL = 0;
    /* check MPU region aligment */
    MPU->RBAR = MPU_RBAR_ADDR_Msk;
    mpu_aligment_mask = ~MPU->RBAR;
    MPU->RBAR = 0;

#ifdef  DEBUG
    dprintf("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
    dprintf("MPU.ALIGNMENT=0x%08X\r\n", mpu_aligment_mask);
    dprintf("MPU.ALIGNMENT_BITS=%i\r\n", mpu_region_bits(mpu_aligment_mask));
#endif/*DEBUG*/
}
