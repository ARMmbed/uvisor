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
#include <debug.h>

static uint32_t g_vmpu_public_flash;

void vmpu_arch_init_hw(void)
{
    /* determine public flash */
    g_vmpu_public_flash = vmpu_acl_static_region(
        0,
        (void*)FLASH_ORIGIN,
        ((uint32_t)__uvisor_config.secure_end)-FLASH_ORIGIN,
        UVISOR_TACLDEF_SECURE_CONST | UVISOR_TACL_EXECUTE
    );

    /* enable box zero SRAM, region can be larger than physical SRAM */
    vmpu_acl_static_region(
        1,
        (void*)0x20000000,
        0X40000,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE
    );
}

int vmpu_addr2pid(uint32_t addr)
{
    return -1;
}
