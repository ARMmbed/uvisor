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
#include <debug.h>
#include <halt.h>
#include <memory_map.h>

void debug_mpu_config(void)
{
    uint32_t dregion, ctrl, size, rasr, ap, tex, srd;
    char dim[][3] = {"B ", "KB", "MB", "GB"};
    int i;

    dprintf("* MPU CONFIGURATION\n\r");

    /* CTRL */
    ctrl = MPU->CTRL;
    dprintf("\n\r");
    dprintf("  Background region %s\n\r", ctrl & MPU_CTRL_PRIVDEFENA_Msk ?
                                          "enabled" : "disabled");
    dprintf("  MPU %s @NMI, @HardFault\n\r", ctrl & MPU_CTRL_HFNMIENA_Msk ?
                                             "enabled" : "bypassed");
    dprintf("  MPU %s\n\r", ctrl & MPU_CTRL_PRIVDEFENA_Msk ?
                                  "enabled" : "disabled");
    dprintf("\n\r");

    /* information for each region (RBAR, RASR) */
    dregion = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    dprintf("  Region Start      Size  XN AP  TEX S C B SRD      Valid\n\r");
    for(i = 0; i < dregion; ++i)
    {
        /* select region */
        MPU->RNR = i;

        /* read RASR fields */
        rasr = MPU->RASR;
        ap   = (rasr & MPU_RASR_AP_Msk)  >> MPU_RASR_AP_Pos;
        tex  = (rasr & MPU_RASR_TEX_Msk) >> MPU_RASR_TEX_Pos;
        srd  = (rasr & MPU_RASR_SRD_Msk) >> MPU_RASR_SRD_Pos;

        /* size is used to compute the actual size in bytes, using the
         * conversion table provided in the ARMv7M manual */
        size = ((rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos) + 1;

        /* print information for region (fields are printed as bits) */
        dprintf("  %d      0x%08X %03d%s %d  %d%d%d %d%d%d %d %d %d ",
                i,
                (MPU->RBAR & MPU_RBAR_ADDR_Msk) >> MPU_RBAR_ADDR_Pos,
                size > 4  ? 0x1 << (size % 10) : 0,
                dim[size / 10],
                (rasr & MPU_RASR_XN_Msk)  >> MPU_RASR_XN_Pos,
                (ap  & 0x1) >> 0x0, (ap  & 0x2) >> 0x1, (ap  & 0x4) >> 0x2,
                (tex & 0x1) >> 0x0, (tex & 0x2) >> 0x1, (tex & 0x4) >> 0x2,
                (rasr & MPU_RASR_S_Msk)   >> MPU_RASR_S_Pos,
                (rasr & MPU_RASR_C_Msk)   >> MPU_RASR_C_Pos,
                (rasr & MPU_RASR_B_Msk)   >> MPU_RASR_B_Pos,
                (rasr & MPU_RASR_SRD_Msk) >> MPU_RASR_B_Pos);
        dprintf("%d%d%d%d%d%d%d%d %d\n\r",
                (srd & 0x01) >> 0x0, (srd & 0x02) >> 0x1,
                (srd & 0x04) >> 0x2, (srd & 0x08) >> 0x3,
                (srd & 0x10) >> 0x4, (srd & 0x20) >> 0x5,
                (srd & 0x40) >> 0x6, (srd & 0x80) >> 0x7,
                rasr & MPU_RASR_ENABLE_Msk ? 1 : 0);
    }
    dprintf("\n\r");
}

void debug_mpu_fault(void)
{
}

void debug_map_addr_to_periph(uint32_t address)
{
}
