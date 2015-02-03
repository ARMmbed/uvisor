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
#ifndef __SVC_GW_H__
#define __SVC_GW_H__

#include <uvisor.h>

#define TO_STR(x)    #x
#define TO_STRING(x) TO_STR(x)

#define SVC_GW_MAGIC       0xABCDABCD /* FIXME update with correct magic */
#define SVC_GW_OFFSET      ((uint8_t) 0x7FU)
#define SVC_GW_BOX_ID_Msk  ((uint8_t) 0x3FU)
#define SVC_GW_BOX_OOP_Msk ((uint8_t) 0x40U)

#define SVC_GW(dst_id, dst_fn)                                                \
    {                                                                        \
        asm volatile(                                                        \
            "svc   %[svc_imm]\n"                                            \
            "b.n   skip_args%=\n"                                           \
            ".word "TO_STRING(SVC_GW_MAGIC)"\n"                             \
            ".word "TO_STRING(dst_fn)"\n"                                   \
            "skip_args%=:\n"                                                \
            :: [svc_imm] "I" (dst_id + SVC_GW_OFFSET + 1)                   \
        );                                                                    \
    }

typedef struct {
    uint16_t opcode;
    uint16_t branch;
    uint32_t magic;
    uint32_t dst_fn;
} PACKED TSecGw;

static inline void svc_gw_check_magic(TSecGw *svc_pc)
{
    if(svc_pc->magic != SVC_GW_MAGIC)
    {
        /* FIXME raise fault */
        while(1);
    }
}

static inline uint32_t svc_gw_get_dst_fn(TSecGw *svc_pc)
{
    return svc_pc->dst_fn;
}

static inline uint8_t svc_gw_get_dst_id(uint8_t svc_imm)
{
    return svc_imm & SVC_GW_BOX_ID_Msk;
}

static inline uint8_t svc_gw_oop_mode(uint8_t svc_imm)
{
    return svc_imm & SVC_GW_BOX_OOP_Msk;
}
#endif/*__SVC_GW_H__*/
