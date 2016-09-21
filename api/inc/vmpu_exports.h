/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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
#ifndef __UVISOR_API_VMPU_EXPORTS_H__
#define __UVISOR_API_VMPU_EXPORTS_H__

#include "api/inc/uvisor_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/rpc_exports.h"
#include <stdint.h>

/* The maximum box namespace length is 37 so that it is exactly big enough for
 * a human-readable hex string GUID (as formatted by RFC 4122) followed by a
 * terminating NULL. */
#define UVISOR_MAX_BOX_NAMESPACE_LENGTH 37

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

#define UVISOR_TACL_EXECUTE         (UVISOR_TACL_UEXECUTE       |\
                                     UVISOR_TACL_SEXECUTE)

/* all possible access control flags */
#define UVISOR_TACL_ACCESS          (UVISOR_TACL_UACL           |\
                                     UVISOR_TACL_SACL)

/* various modes */
#define UVISOR_TACL_STACK           0x0040UL
#define UVISOR_TACL_SIZE_ROUND_UP   0x0080UL
#define UVISOR_TACL_SIZE_ROUND_DOWN 0x0100UL
#define UVISOR_TACL_PERIPHERAL      0x0200UL
#define UVISOR_TACL_SHARED          0x0400UL
#define UVISOR_TACL_USER            0x0800UL
#define UVISOR_TACL_IRQ             0x1000UL

#if defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1

#endif /* defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1 */

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

#define UVISOR_PAD32(x)             (32 - (sizeof(x) & ~0x1FUL))
#define UVISOR_BOX_MAGIC            0x42CFB66FUL
#define UVISOR_BOX_VERSION          100
#define UVISOR_STACK_BAND_SIZE      128
#define UVISOR_MEM_SIZE_ROUND(x)    UVISOR_REGION_ROUND_UP(x)

#define UVISOR_MIN_STACK_SIZE       1024
#define UVISOR_MIN_STACK(x)         (((x) < UVISOR_MIN_STACK_SIZE) ? UVISOR_MIN_STACK_SIZE : (x))

#if defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1

/** Round an address down to the closest 32-byte boundary.
 * @param address[in]   The address to round.
 */
#define UVISOR_ROUND32_DOWN(address) ((address) & ~0x1FUL)

/** Round an address up to the closest 32-byte boundary.
 * @param address[in]   The address to round.
 */
#define UVISOR_ROUND32_UP(address) UVISOR_ROUND32_DOWN((address) + 31UL)


#if defined(ARCH_MPU_ARMv7M)
#define UVISOR_REGION_ROUND_DOWN(x) ((x) & ~((1UL << UVISOR_REGION_BITS(x)) - 1))
#define UVISOR_REGION_ROUND_UP(x)   (1UL << UVISOR_REGION_BITS(x))
#define UVISOR_STACK_SIZE_ROUND(x)  UVISOR_REGION_ROUND_UP(x)
#elif defined(ARCH_MPU_KINETIS)
#define UVISOR_REGION_ROUND_DOWN(x) UVISOR_ROUND32_DOWN(x)
#define UVISOR_REGION_ROUND_UP(x)   UVISOR_ROUND32_UP(x)
#define UVISOR_STACK_SIZE_ROUND(x)  UVISOR_REGION_ROUND_UP((x) + (UVISOR_STACK_BAND_SIZE * 2))
#else
#error "Unknown MPU architecture. uvisor: Check your Makefile. uvisor-lib: Check if uVisor is supported"
#endif

#else /* defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1 */

#define UVISOR_REGION_ROUND_DOWN(x) (x)
#define UVISOR_REGION_ROUND_UP(x)   (x)
#define UVISOR_STACK_SIZE_ROUND(x)  (x)

#endif

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

typedef struct {
    void * param1;
    uint32_t param2;
    UvisorBoxAcl acl;
} UVISOR_PACKED UvisorBoxAclItem;

typedef struct {
    /* Contains user provided size of box context without guards of buffers. */
    uint32_t context_size;
    /* Contains total memory used by the RPC queues (incl. management and pool). */
    uint32_t rpc_outgoing_message_size;
    uint32_t rpc_incoming_message_size;
    uint32_t rpc_fn_group_size;
} UVISOR_PACKED uvisor_sizes_t;

/* The number of additional bss sections per box bss.
 * The size of each section is stored in the box config, and uVisor core will
 * iterate over the box bss, split it into sections as defined by the size table
 * and assign a pointer to beginning of that section into the box index pointer table.
 */
#define UVISOR_BOX_INDEX_SIZE_COUNT (sizeof(uvisor_sizes_t) / sizeof(uint32_t))

typedef struct {
    uint32_t magic;
    uint32_t version;

    /* Box stack size includes stack guards and rounding buffer. */
    uint32_t stack_size;
    /* Contains user provided size of box heap without guards of buffers. */
    uint32_t heap_size;
    /* Contains the size of the index (must be at least sizeof(UvisorBoxIndex)). */
    uint32_t index_size;

    union {
        uint32_t bss_size[UVISOR_BOX_INDEX_SIZE_COUNT];
        uvisor_sizes_t sizes;
    };

    /* Opaque-to-uVisor data that potentially contains uvisor-lib-specific or
     * OS-specific per-box configuration */
    const void * const lib_config;

    const char * box_namespace;
    const UvisorBoxAclItem * const acl_list;
    uint32_t acl_count;
} UVISOR_PACKED UvisorBoxConfig;

typedef struct {
    union {
        void * bss_ptr[UVISOR_BOX_INDEX_SIZE_COUNT];
        struct {
            /* Pointer to the user context */
            void * ctx;
            /* Pointer to the RPC queues */
            uvisor_rpc_outgoing_message_queue_t * rpc_outgoing_message_queue;
            uvisor_rpc_incoming_message_queue_t * rpc_incoming_message_queue;
            uvisor_rpc_fn_group_queue_t * rpc_fn_group_queue;
        };
    };
    /* Pointer to the box heap */
    void * box_heap;
    /* Size of the box heap */
    uint32_t box_heap_size;
    /* Pointer to the currently active heap.
     * This is set to `NULL` by uVisor, signalling to the user lib that the
     * box heap needs to be initialized before use! */
    void * active_heap;

    /* Counter that helps to avoid waiting on the same RPC message result twice
     * by accident. */
    uint32_t rpc_result_counter;

    /* Box ID */
    int box_id_self;

    /* Pointer to the box config */
    const UvisorBoxConfig * config;
} UVISOR_PACKED UvisorBoxIndex;

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

#if defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1
static UVISOR_FORCEINLINE int vmpu_bits(uint32_t size)
{
    return 32 - __builtin_clz(size);
}
#endif /* defined(UVISOR_PRESENT) && UVISOR_PRESENT == 1 */

#endif /* __UVISOR_API_VMPU_EXPORTS_H__ */
