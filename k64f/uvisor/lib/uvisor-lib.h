/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __UVISOR_LIB_H__
#define __UVISOR_LIB_H__

#ifdef __cplusplus
#define UVISOR_EXTERN extern "C"
#else
#define UVISOR_EXTERN extern
#endif

#define UVISOR_BOX_MAGIC 0x42CFB66FUL
#define UVISOR_BOX_VERSION 100
#define UVISOR_PACKED __attribute__((packed))
#define UVISOR_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))
#define UVISOR_ROUND32(x) (((x)+31UL) & ~0x1FUL)
#define UVISOR_PAD32(x) (32 - (sizeof(x) & ~0x1FUL))
#define UVISOR_STACK_BAND_SIZE 128
#define UVISOR_STACK_SIZE_ROUND(size) (UVISOR_ROUND32(size)+(UVISOR_STACK_BAND_SIZE*2))

#ifndef UVISOR_BOX_STACK_SIZE
#define UVISOR_BOX_STACK_SIZE 1024
#endif/*UVISOR_BOX_STACK*/

#define UVISOR_SET_MODE(mode) \
    UVISOR_EXTERN const uint32_t __uvisor_mode = (mode);

#define UVISOR_SECURE_CONST \
    const volatile __attribute__((section(".uvisor.secure"), aligned(32)))

#define UVISOR_SECURE_BSS \
    __attribute__((section(".uvisor.bss.data"), aligned(32)))

#define UVISOR_SECURE_DATA \
    __attribute__((section(".uvisor.data"), aligned(32)))

#define UVISOR_BOX_CONFIG(acl_list, stack_size) \
    \
    uint8_t __attribute__((section(".uvisor.bss.stack"), aligned(32))) \
        acl_list ## _stack[UVISOR_STACK_SIZE_ROUND(stack_size)];\
    \
    static UVISOR_SECURE_CONST UvBoxConfig acl_list ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        sizeof(acl_list ## _stack), \
        acl_list, \
        UVISOR_ARRAY_COUNT(acl_list) \
    }; \
    \
    const __attribute__((section(".uvisor.cfgtbl"), aligned(4))) \
        volatile void* acl_list ## _cfg_ptr  =  & acl_list ## _cfg;

/* FIXME the whole secure gateway will change */
#define TO_STR(x)     #x
#define TO_STRING(x)  TO_STR(x)
#define SVC_GW_MAGIC  0xABCDABCD /* FIXME update with correct magic */
#define SVC_GW_OFFSET ((uint8_t) 0x7FU)

#define secure_gateway(dst_id, dst_fn)                                        \
    ({                                                                        \
        register uint32_t __r0 asm("r0");                                   \
        register uint32_t __r1 asm("r1");                                   \
        register uint32_t __r2 asm("r2");                                   \
        register uint32_t __r3 asm("r3");                                   \
        register uint32_t __res asm("r0");                                  \
        asm volatile(                                                        \
            "svc   %[svc_imm]\n"                                            \
            "b.n   skip_args%=\n"                                           \
            ".word "TO_STRING(SVC_GW_MAGIC)"\n"                             \
            ".word "TO_STRING(dst_fn)"\n"                                   \
            "skip_args%=:\n"                                                \
            :           "=r" (__res)                                        \
            : [svc_imm] "I"  (dst_id + SVC_GW_OFFSET + 1),                  \
                        "r"  (__r0), "r" (__r1), "r" (__r2), "r" (__r3)     \
        );                                                                    \
        __res;                                                                \
     })

typedef uint32_t UvBoxAcl;

typedef struct
{
    const volatile void* start;
    uint32_t length;
    UvBoxAcl acl;
} UVISOR_PACKED UvBoxAclItem;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t stack_size;

    const UvBoxAclItem* const acl_list;
    uint32_t acl_count;
} UVISOR_PACKED UvBoxConfig;

#endif/*__UVISOR_LIB_H__*/
