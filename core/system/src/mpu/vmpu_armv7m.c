/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include <vmpu.h>
#include <halt.h>
#include <debug.h>
#include <memory_map.h>

/* set default MPU region count */
#ifndef ARMv7M_MPU_REGIONS
#define ARMv7M_MPU_REGIONS 8
#endif/*ARMv7M_MPU_REGIONS*/

/* minimum region address alignment */
#ifndef ARMv7M_MPU_ALIGNMENT_BITS
#define ARMv7M_MPU_ALIGNMENT_BITS 5
#endif/*ARMv7M_MPU_ALIGNMENT_BITS*/
#define ARMv7M_MPU_ALIGNMENT_MASK ((1UL<<ARMv7M_MPU_ALIGNMENT_BITS)-1)

static uint32_t g_vmpu_aligment_mask;

void vmpu_acl_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
}

int vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    return 1;
}

static void vmpu_fault_memmanage(void)
{
    DEBUG_FAULT(FAULT_MEMMANAGE);
    halt_led(FAULT_MEMMANAGE);
}

static void vmpu_fault_bus(void)
{
    DEBUG_FAULT(FAULT_BUS);
    halt_led(FAULT_BUS);
}

static void vmpu_fault_usage(void)
{
    DEBUG_FAULT(FAULT_USAGE);
    halt_led(FAULT_USAGE);
}

static void vmpu_fault_hard(void)
{
    DEBUG_FAULT(FAULT_HARD);
    halt_led(FAULT_HARD);
}

static void vmpu_fault_debug(void)
{
    DEBUG_FAULT(FAULT_DEBUG);
    halt_led(FAULT_DEBUG);
}

uint8_t vmpu_bits(uint32_t size)
{
    uint8_t bits=0;
    /* find highest bit */
    while(size)
    {
        size>>=1;
        bits++;
    }
    return bits;
}

void vmpu_dump_settings(void)
{
    uint8_t r;
    DPRINTF("MPU region dump:\r\n");
    for(r=0; r<ARMv7M_MPU_REGIONS; r++)
    {
        MPU->RNR = r;
        DPRINTF("\tR:%i RBAR=0x%08X RASR=0x%08X\r\n", r, MPU->RBAR, MPU->RASR);
    }
    DPRINTF("\tCTRL=0x%08X\r\n", MPU->CTRL);
}

void vmpu_init_protection(void)
{
    /* reset MPU */
    MPU->CTRL = 0;
    /* check MPU region aligment */
    MPU->RBAR = MPU_RBAR_ADDR_Msk;
    g_vmpu_aligment_mask = ~MPU->RBAR;
    MPU->RBAR = 0;

    /* show basic mpu settings */
    DPRINTF("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
    DPRINTF("MPU.ALIGNMENT=0x%08X\r\n", g_vmpu_aligment_mask);
    DPRINTF("MPU.ALIGNMENT_BITS=%i\r\n", vmpu_bits(g_vmpu_aligment_mask));

    /* sanity checks */
    assert(ARMv7M_MPU_REGIONS == ((MPU->TYPE >> MPU_TYPE_DREGION_Pos) & 0xFF));
    assert(ARMv7M_MPU_ALIGNMENT_BITS == vmpu_bits(g_vmpu_aligment_mask));

    /* setup security "bluescreen" exceptions */
    ISR_SET(MemoryManagement_IRQn, &vmpu_fault_memmanage);
    ISR_SET(BusFault_IRQn,         &vmpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &vmpu_fault_usage);
    ISR_SET(HardFault_IRQn,        &vmpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &vmpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;
}
