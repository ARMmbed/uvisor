/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include "vmpu.h"
#include "svc.h"
#include "halt.h"
#include "memory_map.h"
#include "debug.h"

#ifndef MPU_MAX_PRIVATE_FUNCTIONS
#define MPU_MAX_PRIVATE_FUNCTIONS 16
#endif/*MPU_MAX_PRIVATE_FUNCTIONS*/

/* predict SRAM offset */
#ifdef RESERVED_SRAM
#  define RESERVED_SRAM_START UVISOR_ROUND32_UP(SRAM_ORIGIN+RESERVED_SRAM)
#else
#  define RESERVED_SRAM_START SRAM_ORIGIN
#endif

#if (MPU_MAX_PRIVATE_FUNCTIONS>0x100UL)
#error "MPU_MAX_PRIVATE_FUNCTIONS needs to be lower/equal to 0x100"
#endif

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

static void vmpu_fault(int reason)
{
    uint32_t sperr,t;

    /* print slave port details */
    DPRINTF("CESR : 0x%08X\n\r", MPU->CESR);
    sperr = (MPU->CESR >> 27);
    for(t=0; t<5; t++)
    {
        if(sperr & 0x10)
            DPRINTF("  SLAVE_PORT[%i]: @0x%08X (Detail 0x%08X)\n\r",
                t,
                MPU->SP[t].EAR,
                MPU->SP[t].EDR);
        sperr <<= 1;
    }
    DPRINTF("CFSR : 0x%08X\n\r", SCB->CFSR);

    /* doper over and die */
    HALT_ERROR("fault reason %i", reason);
}

static void vmpu_fault_bus(void)
{
    DEBUG_FAULT_BUS();
    DPRINTF("Bus Fault\n\r");
    DPRINTF("BFAR : 0x%08X\n\r", SCB->BFAR);
    vmpu_fault(MPU_FAULT_BUS);
}

static void vmpu_fault_usage(void)
{
    DPRINTF("Usage Fault\n\r");
    vmpu_fault(MPU_FAULT_USAGE);
}

static void vmpu_fault_hard(void)
{
    DPRINTF("Hard Fault\n\r");
    DPRINTF("HFSR : 0x%08X\n\r", SCB->HFSR);
    vmpu_fault(MPU_FAULT_HARD);
}

static void vmpu_fault_debug(void)
{
    DPRINTF("Debug Fault\n\r");
    vmpu_fault(MPU_FAULT_DEBUG);
}

int vmpu_acl_dev(UvisorBoxAcl acl, uint16_t device_id)
{
    return 1;
}

int vmpu_acl_mem(UvisorBoxAcl acl, uint32_t addr, uint32_t size)
{
    return 1;
}

int vmpu_acl_reg(UvisorBoxAcl acl, uint32_t addr, uint32_t rmask,
                 uint32_t wmask)
{
    return 1;
}

int vmpu_acl_bit(UvisorBoxAcl acl, uint32_t addr)
{
    return 1;
}

int vmpu_switch(uint8_t box)
{
    return -1;
}

int vmpu_sanity_checks(void)
{
    /* verify uvisor config structure */
    if(__uvisor_config.magic != UVISOR_MAGIC)
        HALT_ERROR("config magic mismatch: &0x%08X = 0x%08X \
                                 - exptected 0x%08X\n",
                &__uvisor_config,
                __uvisor_config.magic,
                UVISOR_MAGIC);

    /* verify if configuration mode is inside flash memory */
    assert((uint32_t)__uvisor_config.mode >= FLASH_ORIGIN);
    assert((uint32_t)__uvisor_config.mode <= (FLASH_ORIGIN + FLASH_LENGTH - 4));
    DPRINTF("uvisor_mode: %u\n", *__uvisor_config.mode);
    assert(*__uvisor_config.mode <= 2);

    /* verify flash origin and size */
    assert( FLASH_ORIGIN  == 0 );
    assert( __builtin_popcount(FLASH_ORIGIN + FLASH_LENGTH) == 1 );

    /* verify SRAM relocation */
    DPRINTF("uvisor_ram : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.reserved_start,
        VMPU_REGION_SIZE(__uvisor_config.reserved_start,
                         __uvisor_config.reserved_end));
    DPRINTF("             (0x%08X (%u bytes) [linker]\n",
            RESERVED_SRAM_START, USE_SRAM_SIZE);
    assert( __uvisor_config.reserved_end > __uvisor_config.reserved_start );
    assert( VMPU_REGION_SIZE(__uvisor_config.reserved_start,
                             __uvisor_config.reserved_end) == USE_SRAM_SIZE );
    assert(&__stack_end__ <= __uvisor_config.reserved_end);

    assert( (uint32_t) __uvisor_config.reserved_start == RESERVED_SRAM_START);
    assert( (uint32_t) __uvisor_config.reserved_end == (RESERVED_SRAM_START +
                                                        USE_SRAM_SIZE) );

    /* verify that __uvisor_config is within valid flash */
    assert( ((uint32_t) &__uvisor_config) >= FLASH_ORIGIN );
    assert( ((((uint32_t) &__uvisor_config) + sizeof(__uvisor_config))
             <= (FLASH_ORIGIN + FLASH_LENGTH)) );

    /* verify that secure flash area is accessible and after public code */
    assert( __uvisor_config.secure_start <= __uvisor_config.secure_end );
    assert( (uint32_t) __uvisor_config.secure_end <=
            (uint32_t) (FLASH_ORIGIN + FLASH_LENGTH) );
    assert( (uint32_t) __uvisor_config.secure_start >=
            (uint32_t) &vmpu_sanity_checks );

    /* verify configuration table */
    assert( __uvisor_config.cfgtbl_start <= __uvisor_config.cfgtbl_end );
    assert( __uvisor_config.cfgtbl_start >= __uvisor_config.secure_start );
    assert( (uint32_t) __uvisor_config.cfgtbl_end <=
            (uint32_t) (FLASH_ORIGIN + FLASH_LENGTH) );

    /* verify data initialization section */
    assert( __uvisor_config.data_src >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start <= __uvisor_config.data_end );
    assert( __uvisor_config.data_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start >= __uvisor_config.reserved_end );
    assert( (uint32_t) __uvisor_config.data_end <=
            (uint32_t) (SRAM_ORIGIN + SRAM_LENGTH - STACK_SIZE));

    /* verify data bss section */
    assert( __uvisor_config.bss_start <= __uvisor_config.bss_end );
    assert( __uvisor_config.bss_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.bss_start >= __uvisor_config.reserved_end );
    assert( (uint32_t) __uvisor_config.data_end <=
            (uint32_t) (SRAM_ORIGIN + SRAM_LENGTH - STACK_SIZE));

    /* check section ordering */
    assert( __uvisor_config.bss_end <= __uvisor_config.data_start );

    /* return error if uvisor is disabled */
    if(!__uvisor_config.mode || (*__uvisor_config.mode == 0))
        return -1;
    else
        return 0;
}

static void vmpu_init_box_memories(void)
{
    DPRINTF("erasing BSS at 0x%08X (%u bytes)\n",
        __uvisor_config.bss_start,
        VMPU_REGION_SIZE(__uvisor_config.bss_start, __uvisor_config.bss_end)
    );

    /* reset uninitialized secured box data sections */
    memset(
        __uvisor_config.bss_start,
        0,
        VMPU_REGION_SIZE(__uvisor_config.bss_start, __uvisor_config.bss_end)
    );

    DPRINTF("copying .data from 0x%08X to 0x%08X (%u bytes)\n",
        __uvisor_config.data_src,
        __uvisor_config.data_start,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );

    /* initialize secured box data sections */
    memcpy(
        __uvisor_config.data_start,
        __uvisor_config.data_src,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );
}

static void vmpu_add_acl(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
#ifndef NDEBUG
    const MemMap *map;
#endif/*NDEBUG*/

    if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
        size = UVISOR_ROUND32_DOWN(size);
    else
        if(acl & UVISOR_TACL_SIZE_ROUND_UP)
            size = UVISOR_ROUND32_UP(size);

    DPRINTF("\t@0x%08X size=%06i acl=0x%04X [%s]\n", start, size, acl,
        ((map = memory_map_name((uint32_t)start))!=NULL) ? map->name : "unknown"
    );
}

static void vmpu_load_boxes(void)
{
    int i, count;
    uint32_t sp, sp_size;
    const UvisorBoxAclItem *region;
    const UvisorBoxConfig **box_cfgtbl;
    uint8_t box_id;

    /* stack region grows from bss_start downwards */
    sp = UVISOR_ROUND32_DOWN(((uint32_t)__uvisor_config.bss_start) - UVISOR_STACK_BAND_SIZE);

    /* read stack pointer from vector table */
    g_svc_cx_curr_sp[0] = *((uint32_t**)0);
    DPRINTF("box[0] stack pointer = 0x%08X\n", g_svc_cx_curr_sp[0]);

    /* enumerate and initialize boxes */
    g_svc_cx_box_num = 1;
    for(box_cfgtbl = (const UvisorBoxConfig**) __uvisor_config.cfgtbl_start;
        box_cfgtbl < (const UvisorBoxConfig**) __uvisor_config.cfgtbl_end;
        box_cfgtbl++
        )
    {
        /* ensure that configuration resides in flash */
        if(!(VMPU_FLASH_ADDR(*box_cfgtbl) &&
            VMPU_FLASH_ADDR(
                ((uint8_t*)(*box_cfgtbl)) + (sizeof(**box_cfgtbl)-1)
            )))
            HALT_ERROR("invalid address - *box_cfgtbl must point to flash (0x%08X)\n", *box_cfgtbl);

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->magic)!=UVISOR_BOX_MAGIC)
            HALT_ERROR("box[%i] @0x%08X - invalid magic\n",
                g_svc_cx_box_num,
                (uint32_t)(*box_cfgtbl)
            );

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->version)!=UVISOR_BOX_VERSION)
            HALT_ERROR("box[%i] @0x%08X - invalid version (0x%04X!-0x%04X)\n",
                g_svc_cx_box_num,
                *box_cfgtbl,
                (*box_cfgtbl)->version,
                UVISOR_BOX_VERSION
            );

        /* increment box counter */
        if((box_id = g_svc_cx_box_num++)>=SVC_CX_MAX_BOXES)
            HALT_ERROR("box number overflow\n");

        /* load box ACLs in table */
        DPRINTF("box[%i] ACL list:\n", box_id);
        region = (*box_cfgtbl)->acl_list;
        count = (*box_cfgtbl)->acl_count;
        for(i=0; i<count; i++)
        {
            /* ensure that ACL resides in flash */
            if(!VMPU_FLASH_ADDR(region))
                HALT_ERROR("box[i]:acl[i] must be in code section (@0x%08X)\n",
                    g_svc_cx_box_num,
                    i,
                    *box_cfgtbl
                );

            vmpu_add_acl(
                box_id,
                region->start,
                region->length,
                region->acl
            );

            /* proceed to next ACL */
            region++;
        }

        /* set stack pointer to box stack size minus guard band */
        g_svc_cx_curr_sp[box_id] = (uint32_t*)sp;
        /* determine stack extent */
        sp_size = UVISOR_ROUND32_DOWN((*box_cfgtbl)->stack_size);
        sp -= sp_size;
        /* add stack ACL to list */
        vmpu_add_acl(
            box_id,
            (void*)(sp + UVISOR_STACK_BAND_SIZE),
            sp_size - UVISOR_STACK_BAND_SIZE,
            UVISOR_TACL_STACK
        );
    }

    /* check consistency between allocated and actual stack sizes */
    if(sp != (uint32_t)__uvisor_config.reserved_end)
        HALT_ERROR("stack pointers didn't match up: 0x%X != 0x%X\n",
            sp,
            __uvisor_config.reserved_end
        );
    DPRINTF("vmpu_load_boxes [DONE]\n");
}

static void vmpu_fixme(void)
{
    /* FIXME implement security context switch between main and
     *       secure box - hardcoded for now, later using ACLs from
     *       UVISOR_BOX_CONFIG */

    /* the linker script creates a list of all box ACLs configuration
     * pointers from __uvisor_cfgtbl_start to __uvisor_cfgtbl_end */

    AIPS0->PACRM &= ~(1 << 14); // MCG_C1_CLKS (PACRM[15:12])
    AIPS0->PACRN &= ~(1 << 22); // UART0       (PACRN[23:20])
    AIPS0->PACRJ &= ~(1 << 30); // SIM_CLKDIV1 (PACRJ[31:28])
    AIPS0->PACRJ &= ~(1 << 26); // PORTA mux   (PACRJ[27:24])
    AIPS0->PACRJ &= ~(1 << 22); // PORTB mux   (PACRJ[23:20])
    AIPS0->PACRJ &= ~(1 << 18); // PORTC mux   (PACRJ[19:16])
    AIPS0->PACRG &= ~(1 << 2);  // PIT module  (PACRJ[ 3: 0])
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

    /* always initialize protected box memories */
    vmpu_init_box_memories();

    /* load boxes */
    vmpu_load_boxes();

    /* FIXME: remove once vmpu_load_boxes works */
    vmpu_fixme();
}
