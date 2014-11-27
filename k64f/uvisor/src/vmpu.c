#include <uvisor.h>
#include "vmpu.h"

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

static void vmpu_fault(int reason)
{
    uint32_t sperr,t;

    /* print slave port details */
    dprintf("CESR : 0x%08X\n\r", MPU->CESR);
    sperr = (MPU->CESR >> 27);
    for(t=0; t<5; t++)
    {
        if(sperr & 0x10)
            dprintf("  SLAVE_PORT[%i]: @0x%08X (Detail 0x%08X)\n\r",
                t,
                MPU->SP[t].EAR,
                MPU->SP[t].EDR);
        sperr <<= 1;
    }
    dprintf("CFSR : 0x%08X (reason 0x%02x)\n\r", SCB->CFSR, reason);
    while(1);
}

static void vmpu_fault_bus(void)
{
    dprintf("BFAR : 0x%08X\n\r", SCB->BFAR);
    vmpu_fault(MPU_FAULT_BUS);
}

static void vmpu_fault_usage(void)
{
    dprintf("Usage Fault\n\r");
    vmpu_fault(MPU_FAULT_USAGE);
}

static void vmpu_fault_hard(void)
{
    dprintf("HFSR : 0x%08X\n\r", SCB->HFSR);
    vmpu_fault(MPU_FAULT_HARD);
}

static void vmpu_fault_debug(void)
{
    dprintf("MPU_FAULT_DEBUG\n\r");
    vmpu_fault(MPU_FAULT_DEBUG);
}

int vmpu_acl_dev(TACL acl, uint16_t device_id)
{
    return 1;
}

int vmpu_acl_mem(TACL acl, uint32_t addr, uint32_t size)
{
    return 1;
}

int vmpu_acl_reg(TACL acl, uint32_t addr, uint32_t rmask, uint32_t wmask)
{
    return 1;
}

int vmpu_acl_bit(TACL acl, uint32_t addr)
{
    return 1;
}

extern int vmpu_box_add(const TBoxDesc *box)
{
    return 0;
}

void vmpu_init(void)
{
    /* setup security "bluescreen" exceptions */
    ISR_SET(BusFault_IRQn,         &vmpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &vmpu_fault_usage);
    ISR_SET(HardFault_IRQn,        &vmpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &vmpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;
}
