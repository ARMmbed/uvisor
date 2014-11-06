#ifndef __EXPORTS_H__
#define __EXPORTS_H__

/* FIXME a single reset handler is used for all boxes, each of
 *      them executing it separately; it is better to switch
 *      to a different approach */
extern void reset_handler(void);

#define GUID_BYTE_SIZE    16
typedef uint8_t Guid[GUID_BYTE_SIZE];

/* box header table */
typedef struct header_table {
    uint16_t magic;
    uint16_t version;
    uint32_t length;
    uint32_t fn_count;
    uint32_t *fn_table;
    Guid     guid;
} HeaderTable;

extern __attribute__((section(".box_header"))) const HeaderTable g_hdr_tbl;

/* --- TEMPORARY ---
 * boxes relocation:
 *     - each module is relocated in 1 of the 8 subregions of the Flash/SRAM address space
 *     - uVisor will always be box #0
 *     - the uVisor always starts box #1 after reset
 *     - other libraries/modules get higher box #s
 */
#define    UVISOR_BOX    0
#define CLIENT_BOX    1
#define XOR_BOX        2
#define TIMER_BOX    3

/* SVC#s */
#define SVC_SWITCH_IN        0
#define SVC_SWITCH_OUT        1
#define SVC_FIND_BOX_NUM    2
#define SVC_NONBASETHRDENA_IN    3
#define SVC_NONBASETHRDENA_OUT    4
#define SVC_REGISTER_ISR    5

/* header table magic value (to check relocation) */
#define BOX_MAGIC_VALUE    (uint16_t) 0xDEAD

/* code and stack pointers
 * each sub-region has its own
 *     - stack         BOX_STACK(subregion_#)
 *     - entry point        BOX_ENTRY(subregion_#)
 *    - header table        BOX_TABLE(subregion_#    except for the uVisor
 */
#define TABLE_OFFSET    ((uint32_t) 0x0)
#define ENTRY_OFFSET    ((uint32_t) sizeof(HeaderTable))
#define BOX_TABLE(x)    ((HeaderTable *)  ((uint32_t *) (FLASH_MEM_BASE + x * FLASH_SUBREGION_SIZE + TABLE_OFFSET)))
#define BOX_ENTRY(x)    ((uint32_t *)      (FLASH_MEM_BASE + x * FLASH_SUBREGION_SIZE + ENTRY_OFFSET))
#define BOX_STACK(x)    ((uint32_t *)      (RAM_MEM_BASE + (x + 1) * RAM_SUBREGION_SIZE))

/* context switch entry point
 *     - r12 is used to store the accessory argument
 *       ARM ABI calling convention specifies that r12 is preserved
 *       in function calls; on the other hand it is not used to
 *       store arguments in function calls
 *     - functions with an arbitrary number of arguments can be
 *       remapped seamlessly to this entry point:
 *           - #arg <= 4     r0-r3 used,         r12 holds the dst_id (box# and fn#)
 *           - #arg >  4    r0-r3 + stack used, r12 holds the dst_id (box# and fn#) */
static inline uint32_t context_switch_in(uint32_t fn, uint32_t box)
{
    register uint32_t __p0  asm("r12") = (box << 20) | (fn << 8) | SVC_SWITCH_IN;
    register uint32_t __res asm("r0");
    asm volatile(
        "svc    0\n"
        : "=r" (__res)
        : "r" (__p0)
    );
    return __res;
}

/* context switch return point
 *     - r0 holds the result of the called function
 *     FIXME    this only works for 32bit return values */
static inline uint32_t context_switch_out(uint32_t res)
{
    register uint32_t __p0  asm("r12") = SVC_SWITCH_OUT;
    register uint32_t __p1  asm("r0")  = res;
    register uint32_t __res asm("r0");
    asm volatile(
        "svc    0\n"
        : "=r" (__res)
        : "r" (__p0), "r" (__p1)
    );
    return __res;
}

/* request box number for used library */
static inline uint32_t get_box_num(const Guid guid)
{
    register uint32_t __p0  asm("r12") = SVC_FIND_BOX_NUM;
    register uint32_t __p1  asm("r0")  = (uint32_t) guid;
    register uint32_t __res asm("r0");
    asm volatile(
        "svc    0\n"
        : "=r" (__res)
        : "r" (__p0), "r" (__p1)
    );
    return __res;
}

/* ISR set-up by unprivileged code */
static inline uint32_t isr_set_unpriv(uint32_t irqn, void (*hdlr)(void))
{
    register uint32_t __p0  asm("r12") = SVC_REGISTER_ISR;
    register uint32_t __p1  asm("r0")  = IRQn_OFFSET + irqn;
    register uint32_t __p2  asm("r1")  = (uint32_t) hdlr;
    register uint32_t __res asm("r0");
    asm volatile(
        "svc    0\n"
        : "=r" (__res)
        : "r" (__p0), "r" (__p1), "r" (__p2)
    );
    return __res;
}

#endif/*__EXPORTS_H_*/
