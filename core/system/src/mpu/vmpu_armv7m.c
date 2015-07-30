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
#include <svc.h>
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
    uint32_t faults;
} TMpuRegion;

typedef struct {
    TMpuRegion *region;
    uint32_t count;
} TMpuBox;

uint32_t g_mpu_region_count, g_box_mem_pos;
TMpuRegion g_mpu_list[MPU_REGION_COUNT];
TMpuBox g_mpu_box[UVISOR_MAX_BOXES];

static uint32_t g_vmpu_aligment_mask;

int vmpu_switch(uint8_t src_box, uint8_t dst_box)
{
    return 1;
}

/* FIXME check if fault came from ldr/str instruction, check ACL and, if
 * allowed, perform operation and return to regular execution */
void MemoryManagement_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_MEMMANAGE);
    halt_led(FAULT_MEMMANAGE);
}

void BusFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_BUS);
    halt_led(FAULT_BUS);
}

void UsageFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_USAGE);
    halt_led(FAULT_USAGE);
}

void HardFault_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_HARD);
    halt_led(FAULT_HARD);
}

void DebugMonitor_IRQn_Handler(void)
{
    DEBUG_FAULT(FAULT_DEBUG);
    halt_led(FAULT_DEBUG);
}

void vmpu_load_box(uint8_t box_id)
{
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
                    UVISOR_TACL_SREAD;
                flags = MPU_RASR_AP_PRO_URO;
            }
            else
            {
                acl_res =
                    UVISOR_TACL_UREAD |
                    UVISOR_TACL_SREAD | UVISOR_TACL_SWRITE;
                flags = MPU_RASR_AP_PRW_URO;
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
    region->faults = 0;

    DPRINTF("rounded=%05i}\n\r", size_rounded);
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

void vmpu_init_static_regions(void)
{
    debug_mpu_config();

    /* finally enable MPU */
    MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;
}

void vmpu_acl_stack(uint8_t box_id, uint32_t context_size, uint32_t stack_size)
{
    int bits, slots_ctx, slots_stack;
    uint32_t size, block_size;

    /* handle main box */
    if(box_id)
        stack_size = UVISOR_MIN_STACK(stack_size);
    else
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
    if(stack_size<UVISOR_MIN_STACK_SIZE)
        stack_size = UVISOR_MIN_STACK_SIZE;

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
    slots_stack = 8-slots_ctx-1;

    /* final sanity checks */
    if( (slots_ctx * block_size) < context_size )
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_ctx underrun\n\r");
    if( (slots_stack * block_size) < stack_size )
        HALT_ERROR(SANITY_CHECK_FAILED, "slots_stack underrun\n\r");

    /* allocate context pointer */
    g_svc_cx_context_ptr[box_id] = (uint32_t*)g_box_mem_pos;
    /* ensure stack band on top for stack underflow dectection */
    g_svc_cx_curr_sp[box_id] =
        (uint32_t*)(g_box_mem_pos + size - UVISOR_STACK_BAND_SIZE);

    /* create stack protection region */
    vmpu_acl_add(
        box_id, (void*)g_box_mem_pos, size,
        UVISOR_TACLDEF_STACK | UVISOR_TACL_SUBREGIONS(1UL<<slots_ctx)
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
}
