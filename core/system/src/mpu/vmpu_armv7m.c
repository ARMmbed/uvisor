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

void vmpu_init_protection(void)
{
    /* setup security "bluescreen" exceptions */
    ISR_SET(MemoryManagement_IRQn, &vmpu_fault_memmanage);
    ISR_SET(BusFault_IRQn,         &vmpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &vmpu_fault_usage);
    ISR_SET(HardFault_IRQn,        &vmpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &vmpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;
}
