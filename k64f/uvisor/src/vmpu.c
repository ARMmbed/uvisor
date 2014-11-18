#include <uvisor.h>
#include "vmpu.h"

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

static void mpu_fault(int reason)
{
    uint32_t sperr,t;

    /* print slave port details */
    sperr = (MPU->CESR >> 27);
    for(t=0; t<5; t++)
    {
        if(sperr & 0x10)
            dprintf("Slave: @0x%08X (Detail 0x%08X)\n\r",
                MPU->SP[t].EAR,
                MPU->SP[t].EDR);
        sperr >>= 1;
    }
    dprintf("CESR : 0x%08X\n\r", MPU->CESR);
    dprintf("CFSR : 0x%08X (reason 0x%02x)\n\r", SCB->CFSR, reason);
    while(1);
}

static void mpu_fault_bus(void)
{
    dprintf("BFAR : 0x%08X\n\r", SCB->BFAR);
    mpu_fault(MPU_FAULT_BUS);
}

static void mpu_fault_usage(void)
{
    dprintf("Usage Fault\n\r");
    mpu_fault(MPU_FAULT_USAGE);
}

static void mpu_fault_memory_management(void)
{
    dprintf("MMFAR: 0x%08X\n\r", SCB->MMFAR);
    mpu_fault(MPU_FAULT_MEMORY);
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

void vmpu_init(void)
{
    /* setup security "bluescreen" */
    ISR_SET(BusFault_IRQn,         &mpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &mpu_fault_usage);
    ISR_SET(MemoryManagement_IRQn, &mpu_fault_memory_management);
    ISR_SET(HardFault_IRQn,        &mpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &mpu_fault_debug);

}
