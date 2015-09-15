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
#ifndef __VMPU_EXPORTS_H__
#define __VMPU_EXPORTS_H__

/* supervisor user access modes */
#define UVISOR_TACL_UEXECUTE        0x0001UL
#define UVISOR_TACL_UWRITE          0x0002UL
#define UVISOR_TACL_UREAD           0x0004UL
#define UVISOR_TACL_UACL            (UVISOR_TACL_UREAD          |\
                                     UVISOR_TACL_UWRITE         |\
                                     UVISOR_TACL_UEXECUTE)

/* supervisor access modes */
#define UVISOR_TACL_SEXECUTE        0x0008UL
#define UVISOR_TACL_SWRITE          0x0010UL
#define UVISOR_TACL_SREAD           0x0020UL
#define UVISOR_TACL_SACL            (UVISOR_TACL_SREAD          |\
                                     UVISOR_TACL_SWRITE         |\
                                     UVISOR_TACL_SEXECUTE)

#define UVISOR_TACL_EXECUTE (UVISOR_TACL_UEXECUTE | UVISOR_TACL_SEXECUTE)

/* all possible access control flags */
#define UVISOR_TACL_ACCESS          (UVISOR_TACL_UACL | UVISOR_TACL_SACL)

/* various modes */
#define UVISOR_TACL_STACK           0x0040UL
#define UVISOR_TACL_SIZE_ROUND_UP   0x0080UL
#define UVISOR_TACL_SIZE_ROUND_DOWN 0x0100UL
#define UVISOR_TACL_PERIPHERAL      0x0200UL
#define UVISOR_TACL_SHARED          0x0400UL
#define UVISOR_TACL_USER            0x0800UL
#define UVISOR_TACL_IRQ             0x1000UL

/* subregion mask for ARMv7M */
#ifdef  ARCH_MPU_ARMv7M
#define UVISOR_TACL_SUBREGIONS_POS  24
#define UVISOR_TACL_SUBREGIONS_MASK (0xFFUL<<UVISOR_TACL_SUBREGIONS_POS)
#define UVISOR_TACL_SUBREGIONS(x)   ( (((uint32_t)(x))<<UVISOR_TACL_SUBREGIONS_POS) & UVISOR_TACL_SUBREGIONS_MASK )
#endif/*ARCH_MPU_ARMv7M*/

#define UVISOR_TACLDEF_SECURE_BSS   (UVISOR_TACL_UREAD          |\
                                     UVISOR_TACL_UWRITE         |\
                                     UVISOR_TACL_SREAD          |\
                                     UVISOR_TACL_SWRITE         |\
                                     UVISOR_TACL_SIZE_ROUND_UP)

#define UVISOR_TACLDEF_SECURE_CONST (UVISOR_TACL_UREAD          |\
                                     UVISOR_TACL_SREAD          |\
                                     UVISOR_TACL_SIZE_ROUND_UP)

#define UVISOR_TACLDEF_DATA         UVISOR_TACLDEF_SECURE_BSS

#define UVISOR_TACLDEF_PERIPH       (UVISOR_TACL_PERIPHERAL     |\
                                     UVISOR_TACL_UREAD          |\
                                     UVISOR_TACL_UWRITE         |\
                                     UVISOR_TACL_SREAD          |\
                                     UVISOR_TACL_SWRITE         |\
                                     UVISOR_TACL_SIZE_ROUND_UP)

#define UVISOR_TACLDEF_STACK        (UVISOR_TACL_STACK          |\
                                     UVISOR_TACL_UREAD          |\
                                     UVISOR_TACL_UWRITE         |\
                                     UVISOR_TACL_SREAD          |\
                                     UVISOR_TACL_SWRITE)

#define UVISOR_TO_STR(x)            #x
#define UVISOR_TO_STRING(x)         UVISOR_TO_STR(x)
#define UVISOR_ARRAY_COUNT(x)       (sizeof(x)/sizeof(x[0]))
#define UVISOR_PAD32(x)             (32 - (sizeof(x) & ~0x1FUL))
#define UVISOR_BOX_MAGIC            0x42CFB66FUL
#define UVISOR_BOX_VERSION          100
#define UVISOR_STACK_BAND_SIZE      128
#define UVISOR_MEM_SIZE_ROUND(x)    UVISOR_REGION_ROUND_UP(x)

#define UVISOR_MIN_STACK_SIZE       1024
#define UVISOR_MIN_STACK(x)         (((x)<UVISOR_MIN_STACK_SIZE)?UVISOR_MIN_STACK_SIZE:(x))

#ifdef  ARCH_MPU_MK64F
#define UVISOR_REGION_ROUND_DOWN(x) ((x) & ~0x1FUL)
#define UVISOR_REGION_ROUND_UP(x)   UVISOR_REGION_ROUND_DOWN((x)+31UL)
#define UVISOR_STACK_SIZE_ROUND(x)  UVISOR_REGION_ROUND_UP((x) + \
                                    (UVISOR_STACK_BAND_SIZE * 2))
#else /*ARCH_MPU_ARMv7M*/
#define UVISOR_REGION_ROUND_DOWN(x) ((x) & ~((1UL<<UVISOR_REGION_BITS(x))-1))
#define UVISOR_REGION_ROUND_UP(x)   (1UL<<UVISOR_REGION_BITS(x))
#define UVISOR_STACK_SIZE_ROUND(x)  UVISOR_REGION_ROUND_UP(x)
#endif/*ARCH_MPU_ARMv7M*/

#ifndef UVISOR_BOX_STACK_SIZE
#define UVISOR_BOX_STACK_SIZE UVISOR_MIN_STACK_SIZE
#endif/*UVISOR_BOX_STACK*/

/* NOPs added for write buffering synchronization (2 are for dsb. 16bits) */
#define UVISOR_NOP_CNT   (2 + 3)
#define UVISOR_NOP_GROUP \
   "dsb\n" \
   "nop\n" \
   "nop\n" \
   "nop\n"

typedef uint32_t UvisorBoxAcl;

typedef struct
{
    void* param1;
    uint32_t param2;
    UvisorBoxAcl acl;
} UVISOR_PACKED UvisorBoxAclItem;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t stack_size;
    uint32_t context_size;

    const UvisorBoxAclItem* const acl_list;
    uint32_t acl_count;
} UVISOR_PACKED UvisorBoxConfig;

/*
 * only use this macro for rounding const values during compile time:
 * for variables please use uvisor_region_bits(x) instead
 */
#define UVISOR_REGION_BITS(x)       (((x)<=32UL)?5:(((x)<=64UL)?\
    6:(((x)<=128UL)?7:(((x)<=256UL)?8:(((x)<=512UL)?9:(((x)<=1024UL)?\
    10:(((x)<=2048UL)?11:(((x)<=4096UL)?12:(((x)<=8192UL)?\
    13:(((x)<=16384UL)?14:(((x)<=32768UL)?15:(((x)<=65536UL)?\
    16:(((x)<=131072UL)?17:(((x)<=262144UL)?18:(((x)<=524288UL)?\
    19:(((x)<=1048576UL)?20:(((x)<=2097152UL)?21:(((x)<=4194304UL)?\
    22:(((x)<=8388608UL)?23:(((x)<=16777216UL)?24:(((x)<=33554432UL)?\
    25:(((x)<=67108864UL)?26:(((x)<=134217728UL)?27:(((x)<=268435456UL)?\
    28:(((x)<=536870912UL)?29:(((x)<=1073741824UL)?30:(((x)<=2147483648UL)?\
    31:32)))))))))))))))))))))))))))

static inline int vmpu_bits(uint32_t size)
{
    int bits=0;
    /* find highest bit */
    while(size)
    {
        size>>=1;
        bits++;
    }
    return bits;
}

#endif/*__VMPU_EXPORTS_H__*/
