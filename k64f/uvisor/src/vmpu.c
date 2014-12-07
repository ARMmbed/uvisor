#include <uvisor.h>
#include "vmpu.h"

#ifndef MPU_MAX_PRIVATE_FUNCTIONS
#define MPU_MAX_PRIVATE_FUNCTIONS 16
#endif/*MPU_MAX_PRIVATE_FUNCTIONS*/

#if (MPU_MAX_PRIVATE_FUNCTIONS>0x100UL)
#error "MPU_MAX_PRIVATE_FUNCTIONS needs to be lower/equal to 0x100"
#endif

#define MPU_FAULT_USAGE  0x00
#define MPU_FAULT_MEMORY 0x01
#define MPU_FAULT_BUS    0x02
#define MPU_FAULT_HARD   0x03
#define MPU_FAULT_DEBUG  0x04

/* function table hashing support functions */
typedef struct {
    uint32_t addr;
    uint8_t count;
    uint8_t hash;
    uint8_t box_id;
    uint8_t flags;
} TFnTable;

static TFnTable g_fn[MPU_MAX_PRIVATE_FUNCTIONS];
static uint8_t g_fn_hash[0x100];
static int g_fn_count, g_fn_box_count;

static void vmpu_fault(int reason)
{
    uint32_t sperr,t;

    /* print slave port details */
    dprintf("CESR : 0x%08X\n\r", MPU->CESR);
    sperr = (MPU->CESR >> 27);
    for(t=0; t<5; t++)
    {
        if(sperr & 0x10)
            dprintf("  SLAVE_PORT[%i]: @0x%08X (Detail 0x%08X)\n\r",
                t,
                MPU->SP[t].EAR,
                MPU->SP[t].EDR);
        sperr <<= 1;
    }
    dprintf("CFSR : 0x%08X (reason 0x%02x)\n\r", SCB->CFSR, reason);
    while(1);
}

static void vmpu_fault_bus(void)
{
    dprintf("BFAR : 0x%08X\n\r", SCB->BFAR);
    vmpu_fault(MPU_FAULT_BUS);
}

static void vmpu_fault_usage(void)
{
    dprintf("Usage Fault\n\r");
    vmpu_fault(MPU_FAULT_USAGE);
}

static void vmpu_fault_hard(void)
{
    dprintf("HFSR : 0x%08X\n\r", SCB->HFSR);
    vmpu_fault(MPU_FAULT_HARD);
}

static void vmpu_fault_debug(void)
{
    dprintf("MPU_FAULT_DEBUG\n\r");
    vmpu_fault(MPU_FAULT_DEBUG);
}

int vmpu_acl_dev(TACL acl, uint16_t device_id)
{
    return 1;
}

int vmpu_acl_mem(TACL acl, uint32_t addr, uint32_t size)
{
    return 1;
}

int vmpu_acl_reg(TACL acl, uint32_t addr, uint32_t rmask, uint32_t wmask)
{
    return 1;
}

int vmpu_acl_bit(TACL acl, uint32_t addr)
{
    return 1;
}

static uint8_t vmpu_hash_addr(uint32_t data)
{
    return
        (data >> 24) ^
        (data >> 16) ^
        (data >>  8) ^
        data;
}

static int vmpu_box_add_fn(uint8_t box_id, const void **fn, uint32_t count)
{
    TFnTable tmp, *p;
    uint8_t sorting, h, hprev;
    uint32_t addr;
    int i, j;

    /* ensure atomic operation */
    __disable_irq();

    /* add new functions */
    p = &g_fn[g_fn_count];
    for(i=0; i<count; i++)
    {
        /* bail out on table overflow */
        if(g_fn_count>=MPU_MAX_PRIVATE_FUNCTIONS)
        {
            __enable_irq();
            return -1;
        }

        addr = (uint32_t)*fn++;
        p->addr = addr;
        p->hash = vmpu_hash_addr(addr);
        p->box_id = box_id;

        g_fn_count++;
        p++;
    }

    /* silly-sort address hash table & correspondimg addresses */
    do {
        sorting = FALSE;

        for(i=1; i<g_fn_count; i++)
        {
            if(g_fn[i].hash<g_fn[i-1].hash)
            {
                /* swap corresponding adresses */
                tmp = g_fn[i];
                g_fn[i] = g_fn[i-1];
                g_fn[i-1] = tmp;

                sorting = TRUE;
            }
        }
    } while (sorting);

    DPRINTF("added %i functions for box_id=%i:\n", count, box_id);

    /* update g_fn_table_hash and function counts */
    p = g_fn;
    j = 0;
    hprev = g_fn[0].hash;
    for(i=1; i<g_fn_count; i++)
    {
        h = p->hash;
        if(h==hprev)
            p->count = 0;
        else
        {
            /* point hash-table entry to first address in list */
            g_fn_hash[hprev] = j;
            /* remember function count at entry point */
            g_fn[j].count = i-j;

            /* set new entry */
            j = i;
            /* update hash */
            hprev = h;
        }

        DPRINTF("\tfn_addr:0x08X, box:0x%02X, fn_hash=0x%02X, fn_count=0x%02X\n",
            p->addr, p->box_id, p->hash, p->count);

        /* advance to next entry */
        p++;
    }

    /* ensure atomic operation */
    __enable_irq();

    return g_fn_count;
}

int vmpu_box_add(const TBoxDesc *box)
{
    int res;

    if(g_fn_box_count>=0xFF)
        return -1;

    /* incrememt box_id */
    g_fn_box_count++;

    /* add function pointers to global function table */
    if((box->fn_count) &&
        ((res = vmpu_box_add_fn(g_fn_box_count, box->fn_list, box->fn_count))<0))
            return res;

    return 0;
}

int vmpu_fn_box(uint32_t addr)
{
    TFnTable *p;
    uint8_t hash, fn_index, count;

    /* calculate address hash */
    hash = vmpu_hash_addr(addr);

    /* early bail on invalid addresses */
    if((fn_index = g_fn_hash[hash])==0)
        return -1;

    p = &g_fn[fn_index];
    count = p->count;
    do {
        if(p->addr == addr)
            return p->box_id;
        count--;
    } while(count>0);

    return 0;
}

int vmpu_switch(uint8_t box)
{
    return -1;
}

bool vmpu_valid_code_addr(const void* address)
{
    return (((uint32_t)address) >= FLASH_ORIGIN)
        && (((uint32_t)address) <= (uint32_t)__uvisor_config.secure_start);
}

static void vmpu_sanity_checks(void)
{
    /* verify uvisor config structure */
    if(__uvisor_config.magic!=UVISOR_MAGIC)
        while(1)
        {
            DPRINTF("config magic mismatch: &0x%08X=0x%08X - exptected 0x%08X\n",
                &__uvisor_config,
                __uvisor_config.magic,
                UVISOR_MAGIC);
        }

    /* verify if configuration mode is inside flash memory */
    assert((uint32_t)__uvisor_config.mode >= FLASH_ORIGIN);
    assert((uint32_t)__uvisor_config.mode <= (FLASH_ORIGIN+FLASH_LENGTH-4));
    DPRINTF("uvisor_mode: %u\n", *__uvisor_config.mode);
    assert(*__uvisor_config.mode <= 2);

    /* verify SRAM relocation */
    DPRINTF("uvisor_ram : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.reserved_start,
        VMPU_REGION_SIZE(__uvisor_config.reserved_start, __uvisor_config.reserved_end)
    );
    DPRINTF("             (0x%08X (%u bytes) [linker]\n", SRAM_ORIGIN, USE_SRAM_SIZE);
    assert( __uvisor_config.reserved_end > __uvisor_config.reserved_start );
    assert( VMPU_REGION_SIZE(__uvisor_config.reserved_start,__uvisor_config.reserved_end) == USE_SRAM_SIZE );
    assert(&__stack_end__ == __uvisor_config.reserved_end);

    assert( (uint32_t)__uvisor_config.reserved_start == SRAM_ORIGIN);
    assert( (uint32_t)__uvisor_config.reserved_end == (SRAM_ORIGIN+USE_SRAM_SIZE) );

    /* verify that __uvisor_config is within valid flash */
    assert( ((uint32_t)&__uvisor_config)>=FLASH_ORIGIN );
    assert( ((((uint32_t)&__uvisor_config)+sizeof(__uvisor_config))
            <=(FLASH_ORIGIN+FLASH_LENGTH)) );

    /* verify that secure flash area is accessible and after public code */
    assert( __uvisor_config.secure_start <= __uvisor_config.secure_end );
    assert( (uint32_t) __uvisor_config.secure_end <= (uint32_t)(FLASH_ORIGIN+FLASH_LENGTH) );
    assert( (uint32_t) __uvisor_config.secure_start >= (uint32_t)&vmpu_sanity_checks );

    /* verify configuration table */
    assert( __uvisor_config.cfgtbl_start <= __uvisor_config.cfgtbl_end );
    assert( __uvisor_config.cfgtbl_start >= __uvisor_config.secure_start );
    assert( (uint32_t) __uvisor_config.cfgtbl_end <= (uint32_t)(FLASH_ORIGIN+FLASH_LENGTH) );

    /* verify data initialization section */
    assert( __uvisor_config.data_src >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start <= __uvisor_config.data_end );
    assert( __uvisor_config.data_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start >= __uvisor_config.reserved_end );
    assert( (uint32_t)__uvisor_config.data_end <= (uint32_t)(SRAM_ORIGIN+SRAM_LENGTH-STACK_SIZE));

    /* verify data bss section */
    assert( __uvisor_config.bss_start <= __uvisor_config.bss_end );
    assert( __uvisor_config.bss_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.bss_start >= __uvisor_config.reserved_end );
    assert( (uint32_t)__uvisor_config.data_end <= (uint32_t)(SRAM_ORIGIN+SRAM_LENGTH-STACK_SIZE));

    /* check section ordering */
    assert( __uvisor_config.bss_end <= __uvisor_config.data_start );

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
        __uvisor_config.bss_start,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );
    /* initialize secured box data sections */
    memcpy(
        __uvisor_config.data_start,
        __uvisor_config.data_src,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );
}

int vmpu_init(void)
{
    /* run basic sanity checks */
    vmpu_sanity_checks();

    /* bail if uvisor is disabled */
    if(!__uvisor_config.mode || (*__uvisor_config.mode==0))
        return -1;

    /* initialize protected box memories */
    vmpu_init_box_memories();

    /* setup security "bluescreen" exceptions */
    ISR_SET(BusFault_IRQn,         &vmpu_fault_bus);
    ISR_SET(UsageFault_IRQn,       &vmpu_fault_usage);
    ISR_SET(HardFault_IRQn,        &vmpu_fault_hard);
    ISR_SET(DebugMonitor_IRQn,     &vmpu_fault_debug);

    /* enable mem, bus and usage faults */
    SCB->SHCSR |= 0x70000;

    /* return successful */
    return 0;
}
