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
#ifndef __SECURE_GATEWAY_H__
#define __SECURE_GATEWAY_H__

/* FIXME the whole secure gateway will change */
#define secure_gateway(dst_box, dst_fn, a0, a1, a2, a3)                        \
    ({                                                                         \
        register uint32_t __r0  asm("r0") = a0;                                \
        register uint32_t __r1  asm("r1") = a1;                                \
        register uint32_t __r2  asm("r2") = a2;                                \
        register uint32_t __r3  asm("r3") = a3;                                \
        register uint32_t __res asm("r0");                                     \
        asm volatile(                                                          \
            "svc   %[svc_imm]\n"                                               \
            "b.n   skip_args%=\n"                                              \
            ".word "UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC)"\n"                  \
            ".word "UVISOR_TO_STRING(dst_fn)"\n"                               \
            ".word "UVISOR_TO_STRING(dst_box)"_cfg_ptr\n"                      \
            "skip_args%=:\n"                                                   \
            :           "=r" (__res)                                           \
            : [svc_imm] "I"  (UVISOR_SVC_GW_OFFSET + 1),                       \
                        "r"  (__r0), "r" (__r1), "r" (__r2), "r" (__r3)        \
        );                                                                     \
        __res;                                                                 \
     })

#endif/*__SECURE_GATEWAY_H__*/
