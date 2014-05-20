#include <iot-os.h>
#include "mpu.h"

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

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

    /* enforce minimum size */
    if(size<32)
        size = 32;

    /* get region bits */
    if(!(bits = mpu_region_bits(size)))
        return E_PARAM;

    bits--;

    /* verify alignment */
    size = 1UL << bits;
    mask = size-1;
    if(((uint32_t)base) & mask)
        return E_ALIGN;

    /* update MPU */
    MPU->RBAR = MPU_RBAR(region,(uint32_t)base);
    MPU->RASR =
        MPU_RASR_ENABLE_Msk |
        ((bits-1)<<MPU_RASR_SIZE_Pos) |
        flags;

    return 0;
}

static void mpu_fault(int reason)
{
    dprintf("CFSR : 0x%08X\n\r", SCB->CFSR);
    while(1);
}

static void mpu_fault_usage(void)
{
    mpu_fault(MPU_FAULT_USAGE);
}

static void mpu_fault_memory_management(void)
{
    dprintf("MMFAR: 0x%08X\n\r", SCB->MMFAR);
    mpu_fault(MPU_FAULT_MEMORY);
}

static void mpu_fault_bus(void)
{
    dprintf("BFAR : 0x%08X\n\r", SCB->BFAR);
    mpu_fault(MPU_FAULT_BUS);
}

static void mpu_fault_hard(void)
{
    dprintf("HFSR : 0x%08X\n\r", SCB->HFSR);
    mpu_fault(MPU_FAULT_HARD);
}

static void mpu_fault_debug(void)
{
    dprintf("MPU_FAULT_DEBUG\n\r");
    mpu_fault(MPU_FAULT_DEBUG);
}

void mpu_init(void)
{
    /* reset MPU */
    MPU->CTRL = 0;
    /* check MPU region aligment */
    MPU->RBAR = MPU_RBAR_ADDR_Msk;
    mpu_aligment_mask = ~MPU->RBAR;
    MPU->RBAR = 0;

    /* setup security "bluescreen" */
    ISR_SET(BusFault_IRQn,         &mpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &mpu_fault_usage);
    ISR_SET(MemoryManagement_IRQn, &mpu_fault_memory_management);
    ISR_SET(HardFault_IRQn,        &mpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &mpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

#ifdef  DEBUG
    dprintf("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
    dprintf("MPU.ALIGNMENT=0x%08X\r\n", mpu_aligment_mask);
    dprintf("MPU.ALIGNMENT_BITS=%i\r\n", mpu_region_bits(mpu_aligment_mask));
#endif/*DEBUG*/
}
