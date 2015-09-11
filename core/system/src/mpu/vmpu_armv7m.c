/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <uvisor.h>
#include <vmpu.h>
#include <svc.h>
#include <unvic.h>
#include <halt.h>
#include <debug.h>
#include <memory_map.h>

/* set default MPU region count */
#ifndef ARMv7M_MPU_REGIONS
#define ARMv7M_MPU_REGIONS 8
#endif/*ARMv7M_MPU_REGIONS*/

/* set default minimum region address alignment */
#ifndef ARMv7M_MPU_ALIGNMENT_BITS
#define ARMv7M_MPU_ALIGNMENT_BITS 5
#endif/*ARMv7M_MPU_ALIGNMENT_BITS*/

/* derieved region alignment settings */
#define ARMv7M_MPU_ALIGNMENT (1UL<<ARMv7M_MPU_ALIGNMENT_BITS)
#define ARMv7M_MPU_ALIGNMENT_MASK (ARMv7M_MPU_ALIGNMENT-1)

/* MPU helper macros */
#define MPU_RBAR(region,addr)   (((uint32_t)(region))|MPU_RBAR_VALID_Msk|addr)
#define MPU_RBAR_RNR(addr)     (addr)
#define MPU_STACK_GUARD_BAND_SIZE (UVISOR_SRAM_SIZE/8)

/* various MPU flags */
#define MPU_RASR_AP_PNO_UNO (0x00UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_UNO (0x01UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URO (0x02UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URW (0x03UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_UNO (0x05UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_URO (0x06UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_XN         (0x01UL<<MPU_RASR_XN_Pos)
#define MPU_RASR_CB_NOCACHE (0x00UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB_WRA  (0x01UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WT      (0x02UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB      (0x03UL<<MPU_RASR_B_Pos)
#define MPU_RASR_SRD(x)     (((uint32_t)(x))<<MPU_RASR_SRD_Pos)

/* MPU region count */
#ifndef MPU_REGION_COUNT
#define MPU_REGION_COUNT 64
#endif/*MPU_REGION_COUNT*/

typedef struct {
    uint32_t base;
    uint32_t rasr;
    uint32_t size;
    uint32_t acl;
} TMpuRegion;

typedef struct {
    TMpuRegion *region;
    uint32_t count;
} TMpuBox;

uint32_t g_mpu_region_count, g_box_mem_pos;
TMpuRegion g_mpu_list[MPU_REGION_COUNT];
TMpuBox g_mpu_box[UVISOR_MAX_BOXES];

static uint32_t g_vmpu_aligment_mask;

/* TODO/FIXME: implement recovery from MemManage fault */
static int vmpu_fault_recovery_mpu(uint32_t pc, uint32_t sp)
{
    /* FIXME: currently we deny every access */
    /* TODO: implement actual recovery */
    return -1;
}

void vmpu_sys_mux_handler(uint32_t lr)
{
    uint32_t sp, pc;

    /* the IPSR enumerates interrupt numbers from 0 up, while *_IRQn numbers are
     * both positive (hardware IRQn) and negative (system IRQn); here we convert
     * the IPSR value to this latter encoding */
    int ipsr = ((int) (__get_IPSR() & 0x1FF)) - IRQn_OFFSET;

    switch(ipsr)
    {
        case MemoryManagement_IRQn:
            /* currently we only support recovery from unprivileged mode */
            if(lr & 0x4)
            {
                /* pc and sp at fault */
                sp = __get_PSP();
                pc = vmpu_unpriv_uint32_read(sp + (6 * 4));

                /* check if the fault is an MPU fault */
                if((SCB->CFSR & (1 << 7)) && !vmpu_fault_recovery_mpu(pc, sp))
                    return;

                /* if recovery was not successful, throw an error and halt */
                DEBUG_FAULT(FAULT_MEMMANAGE, lr);
                HALT_ERROR(PERMISSION_DENIED, "Access to restricted resource denied");
            }
            else
            {
                DEBUG_FAULT(FAULT_MEMMANAGE, lr);
                HALT_ERROR(FAULT_MEMMANAGE, "Cannot recover from privileged MemManage fault");
            }

        case BusFault_IRQn:
            DEBUG_FAULT(FAULT_BUS, lr);
            halt_led(FAULT_BUS);
            break;

        case UsageFault_IRQn:
            DEBUG_FAULT(FAULT_USAGE, lr);
            halt_led(FAULT_USAGE);
            break;

        case HardFault_IRQn:
            DEBUG_FAULT(FAULT_HARD, lr);
            halt_led(FAULT_HARD);
            break;

        case DebugMonitor_IRQn:
            DEBUG_FAULT(FAULT_DEBUG, lr);
            halt_led(FAULT_DEBUG);
            break;

        default:
            HALT_ERROR(NOT_ALLOWED, "Active IRQn is not a system interrupt");
            break;
    }
}

/* FIXME: added very simple MPU region switching - optimize! */
void vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    int i, mpu_slot;
    const TMpuBox *box;
    const TMpuRegion *region;

    DPRINTF("switching from %i to %i\n\r", src_box, dst_box);

    if(!g_mpu_region_count || dst_box>=UVISOR_MAX_BOXES)
        HALT_ERROR(SANITY_CHECK_FAILED, "dst_box out of range (%u)", dst_box);

    /* handle main box first */
    box = g_mpu_box;
    region = box->region;

    mpu_slot = ARMv7M_MPU_RESERVED_REGIONS;
    for(i=0; i<box->count; i++)
    {
        if(mpu_slot>=ARMv7M_MPU_REGIONS)
             return;
        MPU->RBAR = region->base | mpu_slot | MPU_RBAR_VALID_Msk;
        MPU->RASR = region->rasr;

        /* process next slot */
        region++;
        mpu_slot++;
    }

    /* already updated main box ACLs in previous step */
    if(!dst_box)
        return;

    /* handle target box next */
    box = &g_mpu_box[dst_box];
    region = box->region;

    for(i=0; i<box->count; i++)
    {
        if(mpu_slot>=ARMv7M_MPU_REGIONS)
             return;
        MPU->RBAR = region->base | mpu_slot | MPU_RBAR_VALID_Msk;
        MPU->RASR = region->rasr;

        /* process next slot */
        region++;
        mpu_slot++;
    }

    /* clear remaining slots */
    while(mpu_slot<ARMv7M_MPU_REGIONS)
    {
        MPU->RBAR = mpu_slot | MPU_RBAR_VALID_Msk;
        MPU->RASR = 0;
        mpu_slot++;
    }
}

void vmpu_load_box(uint8_t box_id)
{
    if(box_id == 0)
        vmpu_switch(0, 0);
    else
        HALT_ERROR(NOT_IMPLEMENTED, "currently only box 0 can be loaded");
}

static int vmpu_region_bits(uint32_t size)
{
    int bits;

    bits = vmpu_bits(size)-1;

    /* minimum region size is 32 bytes */
    if(bits<ARMv7M_MPU_ALIGNMENT_BITS)
        bits=ARMv7M_MPU_ALIGNMENT_BITS;

    /* round up if needed */
    if((1UL << bits) != size)
        bits++;

    return bits;
}

static uint32_t vmpu_map_acl(UvisorBoxAcl acl)
{
    uint32_t flags;
    UvisorBoxAcl acl_res;

    /* map generic ACL's to internal ACL's */
    if(acl & UVISOR_TACL_UWRITE)
    {
        acl_res =
            UVISOR_TACL_UREAD | UVISOR_TACL_UWRITE |
            UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
        flags = MPU_RASR_AP_PRW_URW;
    }
    else
        if(acl & UVISOR_TACL_UREAD)
        {
            if(acl & UVISOR_TACL_SWRITE)
            {
                acl_res =
                    UVISOR_TACL_UREAD |
                    UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
                flags = MPU_RASR_AP_PRW_URO;
            }
            else
            {
                acl_res =
                    UVISOR_TACL_UREAD |
                    UVISOR_TACL_SREAD;
                flags = MPU_RASR_AP_PRO_URO;
            }
        }
        else
            if(acl & UVISOR_TACL_SWRITE)
            {
                acl_res =
                    UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
                flags = MPU_RASR_AP_PRW_UNO;
            }
            else
                if(acl & UVISOR_TACL_SREAD)
                {
                    acl_res = UVISOR_TACL_SREAD;
                    flags = MPU_RASR_AP_PRO_UNO;
                }
                else
                {
                    acl_res = 0;
                    flags = MPU_RASR_AP_PNO_UNO;
                }

    /* handle code-execute flag */
    if( acl & (UVISOR_TACL_UEXECUTE|UVISOR_TACL_SEXECUTE) )
        /* can't distinguish between user & supervisor execution */
        acl_res |= UVISOR_TACL_UEXECUTE | UVISOR_TACL_SEXECUTE;
    else
        flags |= MPU_RASR_XN;

    /* check if we meet the expected ACL's */
    if( (acl_res) != (acl & UVISOR_TACL_ACCESS) )
        HALT_ERROR(SANITY_CHECK_FAILED, "inferred ACL's (0x%04X) don't match exptected ACL's (0x%04X)\n\r", acl_res, (acl & UVISOR_TACL_ACCESS));

    return flags;
}

static void vmpu_acl_update_box_region(TMpuRegion *region, uint8_t box_id, void* base, uint32_t size, UvisorBoxAcl acl)
{
    uint32_t flags, bits, mask, size_rounded, subregions;

    DPRINTF("\tbox[%i] acl[%02i]={0x%08X,size=%05i,acl=0x%08X,",
        box_id, g_mpu_region_count, base, size, acl);

    /* verify region alignment */
    bits = vmpu_region_bits(size);
    size_rounded = 1UL << bits;
    if(size_rounded != size)
    {
        if((acl & (UVISOR_TACL_SIZE_ROUND_UP|UVISOR_TACL_SIZE_ROUND_DOWN))==0)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box size (%i) not rounded, rounding disabled (rounded=%i)\n\r",
                size, size_rounded
            );

        if(acl & UVISOR_TACL_SIZE_ROUND_DOWN)
        {
            bits--;
            if(bits<ARMv7M_MPU_ALIGNMENT_BITS)
                HALT_ERROR(SANITY_CHECK_FAILED,
                    "region size (%i) can't be rounded down\n\r",
                    size
                );
            size_rounded = 1UL << bits;
        }
    }

    /* check for correctly aligned base address */
    mask = size_rounded-1;
    if(((uint32_t)base) & mask)
        HALT_ERROR(SANITY_CHECK_FAILED, "base address 0x%08X and size"
        " (%i) are inconsistent\n\r", base, size);

    /* map generic ACL's to internal ACL's */
    flags = vmpu_map_acl(acl);

    /* calculate subregions from ACL */
    subregions =
        ((acl>>UVISOR_TACL_SUBREGIONS_POS) << MPU_RASR_SRD_Pos) &
        MPU_RASR_SRD_Msk;

    /* enable region & add size */
    region->rasr =
        flags | MPU_RASR_ENABLE_Msk |
        ((uint32_t)(bits-1)<<MPU_RASR_SIZE_Pos) |
        subregions;
    region->base = (uint32_t)base;
    region->size = size_rounded;
    region->acl = acl;

    DPRINTF("rounded=%05i}\n\r", size_rounded);
}

uint32_t vmpu_acl_static_region(uint8_t region, void* base, uint32_t size, UvisorBoxAcl acl)
{
    TMpuRegion res;

    /* apply ACL's */
    vmpu_acl_update_box_region(&res, 0, base, size, acl);

    /* apply RASR & RBAR */
    MPU->RBAR = res.base | (region & MPU_RBAR_REGION_Msk) | MPU_RBAR_VALID_Msk;
    MPU->RASR = res.rasr;

    return res.size;
}

void vmpu_acl_add(uint8_t box_id, void* addr, uint32_t size, UvisorBoxAcl acl)
{
    TMpuBox *box;
    TMpuRegion *region;

    if(g_mpu_region_count>=MPU_REGION_COUNT)
        HALT_ERROR(SANITY_CHECK_FAILED, "vmpu_acl_add ran out of regions\n\r");

    if(box_id>=UVISOR_MAX_BOXES)
        HALT_ERROR(SANITY_CHECK_FAILED, "box_id out of range\n\r");

    /* assign box region pointer */
    box = &g_mpu_box[box_id];
    if(!box->region)
        box->region = &g_mpu_list[g_mpu_region_count];

    /* allocate new MPU region */
    region = &box->region[box->count];
    if(region->rasr)
        HALT_ERROR(SANITY_CHECK_FAILED, "unordered region allocation\n\r");

    /* caclulate MPU RASR/BASR registers */
    vmpu_acl_update_box_region(region, box_id, addr, size, acl);

    /* take account for new region */
    box->count++;
    g_mpu_region_count++;
}

void vmpu_acl_stack(uint8_t box_id, uint32_t context_size, uint32_t stack_size)
{
    int bits, slots_ctx, slots_stack;
    uint32_t size, block_size;

    /* handle main box */
    if(!box_id)
    {
        DPRINTF("ctx=%i stack=%i\n\r", context_size, stack_size);
        /* non-important sanity checks */
        assert(context_size == 0);
        assert(stack_size == 0);

        /* assign main box stack pointer to existing
         * unprivileged stack pointer */
        g_svc_cx_curr_sp[0] = (uint32_t*)__get_PSP();
        g_svc_cx_context_ptr[0] = NULL;
        return;
    }

    /* ensure that box stack is at least UVISOR_MIN_STACK_SIZE */
    stack_size = UVISOR_MIN_STACK(stack_size);

    /* ensure that 2/8th are available for protecting stack from
     * context - include rounding error margin */
    bits = vmpu_region_bits(((stack_size + context_size)*8)/6);

    /* ensure MPU region size of at least 256 bytes for
     * subregion support */
    if(bits<8)
        bits = 8;
    size = 1UL << bits;
    block_size = size/8;

    DPRINTF("\tbox[%i] stack=%i context=%i rounded=%i\n\r" , box_id,
        stack_size, context_size, size);

    /* check for correct context address alignment:
     * alignment needs to be a muiltiple of the size */
    if( (g_box_mem_pos & (size-1))!=0 )
        g_box_mem_pos = (g_box_mem_pos & ~(size-1)) + size;

    /* check if we have enough memory left */
    if((g_box_mem_pos + size)>((uint32_t)__uvisor_config.bss_start))
        HALT_ERROR(SANITY_CHECK_FAILED,
            "memory overflow - increase uvisor memory "
            "allocation\n\r");

    /* round context sizes, leave one free slot */
    slots_ctx = (context_size + block_size - 1)/block_size;
    slots_stack = slots_ctx ? (8-slots_ctx-1) : 8;

    /* final sanity checks */
    if( (slots_ctx * block_size) < context_size )
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_ctx underrun\n\r");
    if( (slots_stack * block_size) < stack_size )
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_stack underrun\n\r");

    /* allocate context pointer */
    g_svc_cx_context_ptr[box_id] =
        slots_ctx ? (uint32_t*)g_box_mem_pos : NULL;
    /* ensure stack band on top for stack underflow dectection */
    g_svc_cx_curr_sp[box_id] =
        (uint32_t*)(g_box_mem_pos + size - UVISOR_STACK_BAND_SIZE);

    /* create stack protection region */
    vmpu_acl_add(
        box_id, (void*)g_box_mem_pos, size,
        UVISOR_TACLDEF_STACK | (slots_ctx ? UVISOR_TACL_SUBREGIONS(1UL<<slots_ctx) : 0)
    );

    /* move on to the next memory block */
    g_box_mem_pos += size;
}

void vmpu_arch_init(void)
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

    /* init protected box memory enumation pointer */
    DPRINTF("\n\rbox stack segment start=0x%08X end=0x%08X (length=%i)\n\r",
        __uvisor_config.reserved_end, __uvisor_config.bss_start,
        ((uint32_t)__uvisor_config.bss_start)-((uint32_t)__uvisor_config.reserved_end));
    g_box_mem_pos = (uint32_t)__uvisor_config.reserved_end;

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

    /* initialize static MPU regions */
    vmpu_arch_init_hw();

    /* dump MPU configuration */
#ifndef NDEBUG
    debug_mpu_config();
#endif/*NDEBUG*/

    /* finally enable MPU */
    MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;
}
