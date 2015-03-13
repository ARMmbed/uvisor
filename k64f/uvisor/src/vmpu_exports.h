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
#ifndef __VMPU_EXPORTS_H__
#define __VMPU_EXPORTS_H__

#define UVISOR_TO_STR(x)              #x
#define UVISOR_TO_STRING(x)           UVISOR_TO_STR(x)
#define UVISOR_ARRAY_COUNT(x)         (sizeof(x)/sizeof(x[0]))
#define UVISOR_ROUND32(x)             (((x)+31UL) & ~0x1FUL)
#define UVISOR_PAD32(x)               (32 - (sizeof(x) & ~0x1FUL))
#define UVISOR_BOX_MAGIC              0x42CFB66FUL
#define UVISOR_BOX_VERSION            100
#define UVISOR_STACK_BAND_SIZE        128
#define UVISOR_ROUND_REGION(size)     UVISOR_ROUND32(size)
#define UVISOR_STACK_SIZE_ROUND(size) (UVISOR_ROUND32(size) + \
                                      (UVISOR_STACK_BAND_SIZE * 2))

#ifndef UVISOR_BOX_STACK_SIZE
#define UVISOR_BOX_STACK_SIZE 1024
#endif/*UVISOR_BOX_STACK*/

typedef uint32_t UvisorBoxAcl;

typedef struct
{
    void* start;
    uint32_t length;
    UvisorBoxAcl acl;
} UVISOR_PACKED UvisorBoxAclItem;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t stack_size;

    const UvisorBoxAclItem* const acl_list;
    uint32_t acl_count;
} UVISOR_PACKED UvisorBoxConfig;

#endif/*__VMPU_EXPORTS_H__*/
