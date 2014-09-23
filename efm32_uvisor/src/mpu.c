#include <iot-os.h>
#include "mpu.h"

#define UVISOR_FLASH_BAND (UVISOR_FLASH_SIZE/8)

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

typedef struct {
    uint32_t rbar;
    uint32_t rasr;
} PACKED ACL_MPU_Setting;

typedef struct {
    uint32_t base;            /* base address of access */
    uint32_t mask;            /* mask for determinining access range */
    uint32_t size;            /* region size */
    uint32_t age;            /* last access counter */
    ACL_MPU_Setting mpu;        /* canned MPU region settings */
    uint8_t priority;        /* region priority 0:dynamic, >=1:static */
    uint8_t active;            /* currently assigned MPU region */
    uint8_t size_bits;        /* currently assigned MPU region */
} ACL_Region;

static ACL_Region g_acl_regions[MPU_MAX_ACL];
static int g_dyn_region = 0;
static uint32_t mpu_aligment_mask;
static void* g_config_start;
uint32_t g_config_size;

/* stack and code pointers */
volatile uint32_t * g_sp[MPU_REGION_SPLIT];
volatile uint32_t * g_pc[MPU_REGION_SPLIT];

/* calculate rounded section alignment */
uint8_t mpu_region_bits(uint32_t size)
{
    uint8_t bits=0;

    /* find highest bit */
    while(size)
    {
        size>>=1;
        bits++;
    }

    /* smallest region is 256 bytes for subregion support */
    return (bits<=8)?8:bits;
}

void mpu_debug(void)
{
    uint8_t r;
    dprintf("MPU region dump:\r\n");
    for(r=0; r<MPU_REGIONS; r++)
    {
        MPU->RNR = r;
        dprintf("\tR:%i RBAR=0x%08X RASR=0x%08X\r\n", r, MPU->RBAR, MPU->RASR);
    }
    dprintf("\tCTRL=0x%08X\r\n", MPU->CTRL);
}

/* returns RASR register setting */
static uint32_t mpu_rasr(void* base, uint32_t size, uint32_t flags)
{
    uint8_t bits;
    uint32_t mask;

    /* enforce minimum size */
    if(size<32)
        size = 32;

    /* get region bits */
    if(!(bits = mpu_region_bits(size)))
        return 0;

    bits--;

    /* verify alignment */
    size = 1UL << bits;
    mask = size-1;
    if(((uint32_t)base) & mask)
        return 0;

    /* return MPU RASR register settings */
    return MPU_RASR_ENABLE_Msk | ((bits-1)<<MPU_RASR_SIZE_Pos) | flags;
}

static ACL_Region* mpu_acl_find(void* base)
{
    int i;
    ACL_Region *region = g_acl_regions;

    for(i = 0; i<MPU_MAX_ACL; i++, region++)
        if(region->base == (uint32_t)base)
            return region;

    return NULL;
}

int mpu_acl_set(void* base, uint32_t size, uint8_t priority, uint32_t flags)
{
    uint32_t rasr;
    ACL_Region *region;

    /* caclulate MPU RASR register */
    if(!(rasr = mpu_rasr(base, size, flags)))
        return E_PARAM;

    /* check if MPU entry exists */
    if((region = mpu_acl_find(base)))
    {
        /* allow to erase entry if size == zero */
        if(!size)
        {
            memset(region, 0, sizeof(*region));
            return E_SUCCESS;
        }
    }
    else
    {
        /* if no entry found, allocate a new entry */
        if(!(region = mpu_acl_find(NULL)))
            return E_MEMORY;

        region->base = (uint32_t)base;
    }

    /* update ACL entry */
    region->priority = priority;
    region->mpu.rbar = MPU_RBAR_RNR((uint32_t)base);
    region->mpu.rasr = rasr;
    region->size_bits = ((rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos) + 1;
    region->size = 1UL << region->size_bits;
    region->mask = ~(region->size-1);

    return E_SUCCESS;
}

/* returns rounded MPU section alignment bit count */
int mpu_set(uint8_t region, void* base, uint32_t size, uint32_t flags)
{
    uint32_t rasr;

    /* caclulate MPU RASR register */
    if(!(rasr = mpu_rasr(base, size, flags)))
        return E_PARAM;

    /* update MPU */
    MPU->RBAR = MPU_RBAR(region,(uint32_t)base);
    MPU->RASR = rasr;

    return E_SUCCESS;
}

static void mpu_fault(int reason)
{
    dprintf("CFSR : 0x%08X (reason 0x%02x)\n\r", SCB->CFSR, reason);
    while(1);
}

static void mpu_fault_usage(void)
{
    mpu_fault(MPU_FAULT_USAGE);
}

static void mpu_fault_memory_management(void)
{
    int i;
    ACL_Region *region = g_acl_regions;
    uint32_t address;

    /* check if access is allowed */
    address = SCB->MMFAR;
    for(i = 0; i < MPU_MAX_ACL; i++, region++)
    {
        if((address & region->mask) == region->base)
        {
            dprintf("  caught access to 0x%08X (base=0x%08X) - MPU reprogrammed\n\r", address, region->base);
            /* FIXME remove this */
            if(address == 0xe000ed34) while(1);
            /* update MPU with new region */
            MPU->RNR  = ++g_dyn_region < MPU_DYN_REGIONS ? g_dyn_region : (g_dyn_region = 0);
            MPU->RBAR = region->mpu.rbar;
            MPU->RASR = region->mpu.rasr;

            /* clear MPU fault reason */
            SCB->CFSR = SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk;

            /* allow access */
            return;
        }
    }

    /* bail if access is prohibited */
    dprintf("MMFAR: 0x%08X\n\r", SCB->MMFAR);
    mpu_fault(MPU_FAULT_MEMORY);
}

static void mpu_fault_bus(void)
{
    dprintf("BFAR : 0x%08X\n\r", SCB->BFAR);
    mpu_fault(MPU_FAULT_BUS);
}

static void mpu_fault_hard(void)
{
    dprintf("HFSR : 0x%08X\n\r", SCB->HFSR);
    mpu_fault(MPU_FAULT_HARD);
}

static void mpu_fault_debug(void)
{
    dprintf("MPU_FAULT_DEBUG\n\r");
    mpu_fault(MPU_FAULT_DEBUG);
}

static int mpu_init_uvisor(void)
{
    int res;
    uint32_t t, stack_start;

    /* calculate stack start according to linker file */
    stack_start = ((uint32_t)&__stack_start__) - RAM_MEM_BASE;
    /* round stack start to next stack guard band size */
    t = (stack_start+(STACK_GUARD_BAND_SIZE-1)) & ~(STACK_GUARD_BAND_SIZE-1);

    /* sanity check of stack size */
    stack_start = RAM_MEM_BASE + t;
    if((stack_start>=STACK_POINTER) || ((STACK_POINTER-stack_start)<STACK_MINIMUM_SIZE))
        return E_INVALID;

    /* configure MPU */

    /* calculate guard bands */
    t = (1<<(t/STACK_GUARD_BAND_SIZE)) | 0x80;
    /* protect uvisor RAM, punch holes for guard bands above the stack
     * and below the stack */
    res = mpu_set(7,
        (void*)RAM_MEM_BASE,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PRW_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN|MPU_RASR_SRD(t)
    );

    /* FIXME added SUBREGION_SRAM_SIZE so that sub-region #0 (16KB) is protected */
    /* set uvisor RAM guard bands to RO & inaccessible to all code */
    res|= mpu_set(6,
        (void*)RAM_MEM_BASE,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PNO_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    /* protect uvisor flash-data from unprivileged code,
     * poke holes for code, only protect remaining data blocks,
     * set XN for stored data */
    g_config_start = (void*)((((uint32_t)&__code_end__)+(UVISOR_FLASH_BAND-1)) & ~(UVISOR_FLASH_BAND-1));
    t = (((uint32_t)g_config_start)-FLASH_MEM_BASE)/UVISOR_FLASH_BAND;
    g_config_size = UVISOR_FLASH_SIZE - (t*UVISOR_FLASH_BAND);
    res|= mpu_set(5,
        (void*)FLASH_MEM_BASE,
        UVISOR_FLASH_SIZE,
        MPU_RASR_AP_PRO_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN|MPU_RASR_SRD((1<<t)-1)
    );

    /* create mask for SRD: all regions disabled except for
     *   #0 (uvisor)
     *   #1 (unprivileged box) */
    t = 0xFC;
    /* allow access to flash */
    res|= mpu_set(4,
        (void*)FLASH_MEM_BASE,
        FLASH_SIZE,
        MPU_RASR_AP_PRO_URO|MPU_RASR_CB_WT|MPU_RASR_SRD(t)
    );

    /* create mask for SRD: all regions disabled except for
     *   #0 (uvisor)
     *   #1 (unprivileged box) */
    t = 0xFC;
    /* allow access to unprivileged user SRAM */
    res|= mpu_set(3,
        (void*)RAM_MEM_BASE,
        SRAM_SIZE,
        MPU_RASR_AP_PRW_URW|MPU_RASR_CB_WB_WRA|MPU_RASR_SRD(t)
    );

    mpu_debug();

    /* finally enable MPU */
    if(!res)
        MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;

    return res ? E_ALIGN:E_SUCCESS;
}

void mpu_acl_debug(void)
{
    int i;
    ACL_Region *region = g_acl_regions;

    dprintf("MPU ACL list - maximum of %i entries:\n",MPU_MAX_ACL);
    for(i = 0; i<MPU_MAX_ACL; i++, region++)
        if(region->base)
            dprintf("  %02i: @0x%08X size=0x%04X pri=%i\n",
                i,
                region->base,
                region->size,
                region->priority);
}

/* initilize stack pointers for the whole SRAM region */
void mpu_init_ptrs()
{
    int i;

    for(i = 0; i < MPU_REGION_SPLIT; i++)
    {
        g_sp[i] = MPU_STACK(i);
        g_pc[i] = MPU_CODE(i);
    }
}

int mpu_init(void)
{
    int res;

    /* reset ACL region list */
    memset(&g_acl_regions, 0, sizeof(g_acl_regions));

    /* reset MPU */
    MPU->CTRL = 0;
    /* check MPU region aligment */
    MPU->RBAR = MPU_RBAR_ADDR_Msk;
    mpu_aligment_mask = ~MPU->RBAR;
    MPU->RBAR = 0;

    /* setup security "bluescreen" */
    ISR_SET(BusFault_IRQn,         &mpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &mpu_fault_usage);
    ISR_SET(MemoryManagement_IRQn, &mpu_fault_memory_management);
    ISR_SET(HardFault_IRQn,        &mpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &mpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

    /* initialize code and stack pointers */
    mpu_init_ptrs();

    /* configure uvisor MPU configuration */
    res = mpu_init_uvisor();

#ifdef  DEBUG
    dprintf("MPU.REGIONS=%i\r\n", (uint8_t)(MPU->TYPE >> MPU_TYPE_DREGION_Pos));
    dprintf("MPU.ALIGNMENT=0x%08X\r\n", mpu_aligment_mask);
    dprintf("MPU.ALIGNMENT_BITS=%i\r\n", mpu_region_bits(mpu_aligment_mask));
#endif/*DEBUG*/

    return res;
}
