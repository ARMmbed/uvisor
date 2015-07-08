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

static uint8_t vmpu_bits(uint32_t size)
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

void vmpu_load_box(uint8_t box_id)
{
}

/* returns RASR register setting */
static uint32_t vmpu_rasr(void* base, uint32_t size, uint32_t flags)
{
    uint32_t bits, mask, size_rounded;

    /* enforce minimum size */
    if(size>ARMv7M_MPU_ALIGNMENT)
        bits = vmpu_bits(size);
    else
    {
        size = ARMv7M_MPU_ALIGNMENT;
        bits = ARMv7M_MPU_ALIGNMENT_BITS;
    }

    /* verify region alignment */
    size_rounded = 1UL << bits;
    /* check for correctly aligned base address */
    mask = size_rounded-1;
    if(((uint32_t)base) & mask)
        return 0;

    /* return MPU RASR register settings */
    return MPU_RASR_ENABLE_Msk | ((bits-1)<<MPU_RASR_SIZE_Pos) | flags;
}

/* returns rounded MPU section alignment bit count */
int vmpu_set(uint8_t region, void* base, uint32_t size, uint32_t flags)
{
    uint32_t rasr;

    /* caclulate MPU RASR register */
    if(!(rasr = vmpu_rasr(base, size, flags)))
        return E_PARAM;

    /* update MPU */
    MPU->RBAR = MPU_RBAR(region,(uint32_t)base);
    MPU->RASR = rasr;

    return E_SUCCESS;
}

int vmpu_init_static_regions(void)
{
    int res;
    uint32_t t, stack_start;

    /* calculate stack start according to linker file */
    stack_start = ((uint32_t)&__stack_start__) - SRAM_ORIGIN;
    /* round stack start to next stack guard band size */
    t = (stack_start+(MPU_STACK_GUARD_BAND_SIZE-1)) & ~(MPU_STACK_GUARD_BAND_SIZE-1);

    /* FIXME: sanity check of stack size & pointer */

    /* configure MPU */

    /* calculate guard bands */
    t = (1<<(t/MPU_STACK_GUARD_BAND_SIZE)) | 0x80;

    /* protect uvisor RAM, punch holes for guard bands above the stack
     * and below the stack */
    res = vmpu_set(7,
        (void*)SRAM_ORIGIN,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PRW_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN|MPU_RASR_SRD(t)
    );

    /* set uvisor RAM guard bands to RO & inaccessible to all code */
    res|= vmpu_set(6,
        (void*)SRAM_ORIGIN,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PNO_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    /* protect uvisor flash-data from unprivileged code,
     * poke holes for code, only protect remaining data blocks,
     * set XN for stored data */
    res|= vmpu_set(5,
        (void*)FLASH_ORIGIN,
        UVISOR_FLASH_SIZE,
        MPU_RASR_AP_PRO_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    /* allow access to flash */
    res|= vmpu_set(4,
        (void*)FLASH_ORIGIN,
        FLASH_LENGTH,
        MPU_RASR_AP_PRO_URO|MPU_RASR_CB_WT
    );

    /* allow access to unprivileged user SRAM */
    res|= vmpu_set(3,
        (void*)SRAM_ORIGIN,
        SRAM_LENGTH,
        MPU_RASR_AP_PRW_URW|MPU_RASR_CB_WB_WRA
    );

    debug_mpu_config();

    /* finally enable MPU */
    if(!res)
        MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;

    return res ? E_ALIGN:E_SUCCESS;
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
