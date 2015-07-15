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

static void vmpu_load_boxes(void)
{
    int i, count;
    uint32_t sp, sp_size;
    const UvisorBoxAclItem *region;
    const UvisorBoxConfig **box_cfgtbl;
    uint8_t box_id;

    /* stack region grows from bss_start downwards */
    sp = UVISOR_ROUND32_DOWN(((uint32_t)__uvisor_config.bss_start) -
         UVISOR_STACK_BAND_SIZE);

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
            vmpu_acl_add(
                box_id,
                (void*)(sp + UVISOR_STACK_BAND_SIZE),
                sp_size - UVISOR_STACK_BAND_SIZE,
                UVISOR_TACLDEF_STACK
            );
        }
        DPRINTF("\tbox[%i] stack pointer = 0x%08X\n",
            box_id,
            g_svc_cx_curr_sp[box_id]
        );

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

                /* add ACL, and force entry as user-provided */
                vmpu_acl_add(
                    box_id,
                    region->start,
                    region->length,
                    region->acl | UVISOR_TACL_USER
                );

                /* proceed to next ACL */
                region++;
            }
        }
    }

    /* check consistency between allocated and actual stack sizes */
    if(sp != (uint32_t)__uvisor_config.reserved_end)
        HALT_ERROR(SANITY_CHECK_FAILED,
            "stack pointers didn't match up: 0x%X != 0x%X\n",
            sp,
            __uvisor_config.reserved_end
        );

    /* load box 0 */
    vmpu_load_box(0);

    DPRINTF("vmpu_load_boxes [DONE]\n");
}

void UVISOR_WEAK vmpu_load_box(uint8_t box_id)
{
    HALT_ERROR(NOT_IMPLEMENTED,
               "vmpu_load_box needs hw-specific implementation\n\r");
}

int UVISOR_WEAK vmpu_acl_dev(UvisorBoxAcl acl, uint16_t device_id)
{
    HALT_ERROR(NOT_IMPLEMENTED, "vmpu_acl_dev not implemented yet\n\r");
    return 1;
}

int UVISOR_WEAK vmpu_acl_mem(UvisorBoxAcl acl, uint32_t addr, uint32_t size)
{
    HALT_ERROR(NOT_IMPLEMENTED, "vmpu_acl_mem not implemented yet\n\r");
    return 1;
}

int UVISOR_WEAK vmpu_acl_reg(UvisorBoxAcl acl, uint32_t addr, uint32_t rmask,
                             uint32_t wmask)
{
    HALT_ERROR(NOT_IMPLEMENTED, "vmpu_acl_reg not implemented yet\n\r");
    return 1;
}

int UVISOR_WEAK vmpu_acl_bit(UvisorBoxAcl acl, uint32_t addr)
{
    HALT_ERROR(NOT_IMPLEMENTED, "vmpu_acl_bit not implemented yet\n\r");
    return 1;
}

void UVISOR_WEAK vmpu_acl_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl)
{
    HALT_ERROR(NOT_IMPLEMENTED,
               "vmpu_acl_add needs hw-specific implementation\n\r");
}

int UVISOR_WEAK vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    HALT_ERROR(NOT_IMPLEMENTED,
               "vmpu_switch needs hw-specific implementation\n\r");
    return 1;
}

void UVISOR_WEAK vmpu_init_protection(void)
{
    HALT_ERROR(NOT_IMPLEMENTED,
               "vmpu_init_protection needs hw-specific implementation\n\r");
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

void vmpu_init_post(void)
{
    /* enable non-base thread mode */
    /* exceptions can now return to thread mode regardless their origin
     * (supervisor or thread mode); the opposite is not true */
    SCB->CCR |= 1;

    /* init memory protection */
    vmpu_init_protection();

    /* load boxes */
    vmpu_load_boxes();
}
