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
#include "vmpu_aips.h"
#include "vmpu_mem.h"
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
    HALT_ERROR(reason, "fault reason %i", reason);
}

static void vmpu_fault_bus(void)
{
    DEBUG_FAULT_BUS();
    DPRINTF("Bus Fault\n\r");
    DPRINTF("BFAR : 0x%08X\n\r", SCB->BFAR);
    vmpu_fault(FAULT_BUS);
}

static void vmpu_fault_usage(void)
{
    DEBUG_FAULT_USAGE();
    DPRINTF("Usage Fault\n\r");
    vmpu_fault(FAULT_USAGE);
}

static void vmpu_fault_hard(void)
{
    DEBUG_FAULT_HARD();
    DPRINTF("Hard Fault\n\r");
    DPRINTF("HFSR : 0x%08X\n\r", SCB->HFSR);
    vmpu_fault(FAULT_HARD);
}

static void vmpu_fault_debug(void)
{
    DPRINTF("Debug Fault\n\r");
    vmpu_fault(FAULT_DEBUG);
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

int vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    /* switch ACLs for peripherals */
    vmpu_aips_switch(src_box, dst_box);

    /* switch ACLs for memory regions */
    /* vmpu_mem_switch(src_box, dst_box); */

    return 0;
}

static int vmpu_sanity_checks(void)
{
    /* verify uvisor config structure */
    if(__uvisor_config.magic != UVISOR_MAGIC)
        HALT_ERROR(SANITY_CHECK_FAILED,
            "config magic mismatch: &0x%08X = 0x%08X \
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

int vmpu_init_pre(void)
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

    return vmpu_sanity_checks();
}

static void vmpu_add_acl(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    int res;

#ifndef NDEBUG
    const MemMap *map;
#endif/*NDEBUG*/

    /* check for maximum box ID */
    if(box_id>=UVISOR_MAX_BOXES)
        HALT_ERROR(SANITY_CHECK_FAILED, "box ID out of range (%i)\n", box_id);

    /* check for alignment to 32 bytes */
    if(((uint32_t)start) & 0x1F)
        HALT_ERROR(SANITY_CHECK_FAILED, "ACL start address is not aligned [0x%08X]\n", start);

    /* round ACLs if needed */
    if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
        size = UVISOR_ROUND32_DOWN(size);
    else
        if(acl & UVISOR_TACL_SIZE_ROUND_UP)
            size = UVISOR_ROUND32_UP(size);

    DPRINTF("\t@0x%08X size=%06i acl=0x%04X [%s]\n", start, size, acl,
        ((map = memory_map_name((uint32_t)start))!=NULL) ? map->name : "unknown"
    );

    /* check for peripheral memory, proceed with general memory */
    if(acl & UVISOR_TACL_PERIPHERAL)
        res = vmpu_add_aips(box_id, start, size, acl);
    else
        res = vmpu_add_mem(box_id, start, size, acl);

    if(!res)
        HALT_ERROR(NOT_ALLOWED, "ACL in unhandled memory area\n");
    else
        if(res<0)
            HALT_ERROR(SANITY_CHECK_FAILED, "ACL sanity check failed [%i]\n", res);
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

    /* enumerate and initialize boxes */
    g_svc_cx_box_num = 0;
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
            HALT_ERROR(SANITY_CHECK_FAILED,
                "invalid address - \
                *box_cfgtbl must point to flash (0x%08X)\n", *box_cfgtbl);

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->magic)!=UVISOR_BOX_MAGIC)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid magic\n",
                g_svc_cx_box_num,
                (uint32_t)(*box_cfgtbl)
            );

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->version)!=UVISOR_BOX_VERSION)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid version (0x%04X!-0x%04X)\n",
                g_svc_cx_box_num,
                *box_cfgtbl,
                (*box_cfgtbl)->version,
                UVISOR_BOX_VERSION
            );

        /* increment box counter */
        if((box_id = g_svc_cx_box_num++)>=UVISOR_MAX_BOXES)
            HALT_ERROR(SANITY_CHECK_FAILED, "box number overflow\n");

        /* load box ACLs in table */
        DPRINTF("box[%i] ACL list:\n", box_id);
        region = (*box_cfgtbl)->acl_list;
        if(region)
        {
            count = (*box_cfgtbl)->acl_count;
            for(i=0; i<count; i++)
            {
                /* ensure that ACL resides in flash */
                if(!VMPU_FLASH_ADDR(region))
                    HALT_ERROR(SANITY_CHECK_FAILED,
                        "box[%i]:acl[%i] must be in code section (@0x%08X)\n",
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
        }

        /* update stack pointers */
        if(!box_id)
            g_svc_cx_curr_sp[0] = (uint32_t*)__get_PSP();
        else
        {
            /* set stack pointer to box stack size minus guard band */
            g_svc_cx_curr_sp[box_id] = (uint32_t*)sp;
            /* determine stack extent */
            sp_size = UVISOR_ROUND32_DOWN((*box_cfgtbl)->stack_size);
            if(sp_size <= (UVISOR_STACK_BAND_SIZE+4))
                HALT_ERROR(SANITY_CHECK_FAILED,
                    "box[%i] stack too small (%i)\n",
                    box_id,
                    sp_size
                );
            sp -= sp_size;
            /* add stack ACL to list */
            vmpu_add_acl(
                box_id,
                (void*)(sp + UVISOR_STACK_BAND_SIZE),
                sp_size - UVISOR_STACK_BAND_SIZE,
                UVISOR_TACLDEF_STACK
            );
        }
        DPRINTF("box[%i] stack pointer = 0x%08X\n",
            box_id,
            g_svc_cx_curr_sp[box_id]
        );
    }

    /* check consistency between allocated and actual stack sizes */
    if(sp != (uint32_t)__uvisor_config.reserved_end)
        HALT_ERROR(SANITY_CHECK_FAILED,
            "stack pointers didn't match up: 0x%X != 0x%X\n",
            sp,
            __uvisor_config.reserved_end
        );
    DPRINTF("vmpu_load_boxes [DONE]\n");
}

void vmpu_init_post(void)
{
    /* setup security "bluescreen" exceptions */
    ISR_SET(BusFault_IRQn,         &vmpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &vmpu_fault_usage);
    ISR_SET(HardFault_IRQn,        &vmpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &vmpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

    /* enable non-base thread mode */
    /* exceptions can now return to thread mode regardless their origin
     * (supervisor or thread mode); the opposite is not true */
    SCB->CCR |= 1;

    /* load boxes */
    vmpu_load_boxes();

    /* load ACLs for box 0 */
    vmpu_aips_switch(0, 0);
}
