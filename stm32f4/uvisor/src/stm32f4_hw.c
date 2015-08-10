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

#define VMPU_GRANULARITY 0x400

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
        0x40000,
        UVISOR_TACLDEF_DATA | UVISOR_TACL_EXECUTE
    );
}

int vmpu_addr2pid(uint32_t addr)
{
    uint8_t block = (uint8_t)(addr>>16);
    uint16_t block_low = (uint16_t)addr;

    switch((uint8_t)(addr>>24))
    {
        /* APB1, APB2, AHB1 */
        case 0x40:
            /* detect peripheral region ID 0x00-0xBF */
            if(block<=0x02)
                return (addr & 0x3FFFFUL)/VMPU_GRANULARITY;

            /* USB OTG HS */
            if((block>0x04) && (block<=0x07))
                return 0xC0;

            /* bail out if no match */
            break;

        /* FSMC or FMC */
        case 0xA0:
            return ((!block) && (block_low <= 0xFFF)) ? 0xC1 : -1;

        /* AHB2 */
        case 0x50:
            switch(block)
            {
                /* USB OTG FS */
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                    return 0xC2;

                /* DMCI */
                case 0x05:
                    return (block_low <= 0x3FF) ? 0xC3 : -1;

                /* crypto peripheral region ID 0xC4-0xC6 */
                case 0x06:
                    return (block_low <= 0xBFF) ?
                        0xC4+(block_low/VMPU_GRANULARITY) : -1;
            }
    }

    /* default is invalid peripheral */
    return -1;
}
