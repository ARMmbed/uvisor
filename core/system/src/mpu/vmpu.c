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
#include <uvisor.h>
#include "vmpu.h"
#include "svc.h"
#include "halt.h"
#include "memory_map.h"
#include "debug.h"

#ifndef MPU_MAX_PRIVATE_FUNCTIONS
#define MPU_MAX_PRIVATE_FUNCTIONS 16
#endif/*MPU_MAX_PRIVATE_FUNCTIONS*/

/* predict SRAM offset */
#ifdef RESERVED_SRAM
#  define RESERVED_SRAM_START UVISOR_REGION_ROUND_UP(SRAM_ORIGIN+RESERVED_SRAM)
#else
#  define RESERVED_SRAM_START SRAM_ORIGIN
#endif

#if (MPU_MAX_PRIVATE_FUNCTIONS>0x100UL)
#error "MPU_MAX_PRIVATE_FUNCTIONS needs to be lower/equal to 0x100"
#endif

uint32_t  g_vmpu_box_count;

static int vmpu_sanity_checks(void)
{
    /* verify uvisor config structure */
    if(__uvisor_config.magic != UVISOR_MAGIC)
        HALT_ERROR(SANITY_CHECK_FAILED,
            "config magic mismatch: &0x%08X = 0x%08X \
                                  - exptected 0x%08X\n",
            &__uvisor_config,
            __uvisor_config.magic,
            UVISOR_MAGIC);

    /* verify basic assumptions about vmpu_bits/__builtin_clz */
    assert(__builtin_clz(0) == 32);
    assert(__builtin_clz(1UL << 31) == 0);
    assert(vmpu_bits(0) == 0);
    assert(vmpu_bits(1UL << 31) == 32);
    assert(vmpu_bits(0x8000UL) == 16);
    assert(vmpu_bits(0x8001UL) == 16);
    assert(vmpu_bits(1) == 1);

    /* verify if configuration mode is inside flash memory */
    assert((uint32_t)__uvisor_config.mode >= FLASH_ORIGIN);
    assert((uint32_t)__uvisor_config.mode <= (FLASH_ORIGIN + FLASH_LENGTH - 4));
    DPRINTF("uvisor_mode: %u\n", *__uvisor_config.mode);
    assert(*__uvisor_config.mode <= 2);

    /* verify SRAM relocation */
    DPRINTF("uvisor_ram : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.reserved_start,
        VMPU_REGION_SIZE(__uvisor_config.reserved_start,
                         __uvisor_config.reserved_end));
    DPRINTF("             (0x%08X (%u bytes) [linker]\n",
            RESERVED_SRAM_START, USE_SRAM_SIZE);
    assert( __uvisor_config.reserved_end > __uvisor_config.reserved_start );
    assert( VMPU_REGION_SIZE(__uvisor_config.reserved_start,
                             __uvisor_config.reserved_end) == USE_SRAM_SIZE );
    assert(&__stack_end__ <= __uvisor_config.reserved_end);

    assert( (uint32_t) __uvisor_config.reserved_start == RESERVED_SRAM_START);
    assert( (uint32_t) __uvisor_config.reserved_end == (RESERVED_SRAM_START +
                                                        USE_SRAM_SIZE) );

    /* verify that __uvisor_config is within valid flash */
    assert( ((uint32_t) &__uvisor_config) >= FLASH_ORIGIN );
    assert( ((((uint32_t) &__uvisor_config) + sizeof(__uvisor_config))
             <= (FLASH_ORIGIN + FLASH_LENGTH)) );

    /* verify that secure flash area is accessible and after public code */
    assert( __uvisor_config.secure_start <= __uvisor_config.secure_end );
    assert( (uint32_t) __uvisor_config.secure_end <=
            (uint32_t) (FLASH_ORIGIN + FLASH_LENGTH) );
    assert( (uint32_t) __uvisor_config.secure_start >=
            (uint32_t) &vmpu_sanity_checks );

    /* verify configuration table */
    assert( __uvisor_config.cfgtbl_start <= __uvisor_config.cfgtbl_end );
    assert( __uvisor_config.cfgtbl_start >= __uvisor_config.secure_start );
    assert( (uint32_t) __uvisor_config.cfgtbl_end <=
            (uint32_t) (FLASH_ORIGIN + FLASH_LENGTH) );

    /* verify data initialization section */
    assert( __uvisor_config.data_src >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start <= __uvisor_config.data_end );
    assert( __uvisor_config.data_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.data_start >= __uvisor_config.reserved_end );
    assert( (uint32_t) __uvisor_config.data_end <=
            (uint32_t) (SRAM_ORIGIN + SRAM_LENGTH - STACK_SIZE));

    /* verify data bss section */
    assert( __uvisor_config.bss_start <= __uvisor_config.bss_end );
    assert( __uvisor_config.bss_start >= __uvisor_config.secure_start );
    assert( __uvisor_config.bss_start >= __uvisor_config.reserved_end );
    assert( (uint32_t) __uvisor_config.data_end <=
            (uint32_t) (SRAM_ORIGIN + SRAM_LENGTH - STACK_SIZE));

    /* check section ordering */
    assert( __uvisor_config.bss_end <= __uvisor_config.data_start );

    /* return error if uvisor is disabled */
    if(!__uvisor_config.mode || (*__uvisor_config.mode == 0))
        return -1;
    else
        return 0;
}

static void vmpu_load_boxes(void)
{
    int i, count;
    const UvisorBoxAclItem *region;
    const UvisorBoxConfig **box_cfgtbl;
    uint8_t box_id;

    /* enumerate and initialize boxes */
    g_vmpu_box_count = 0;
    for(box_cfgtbl = (const UvisorBoxConfig**) __uvisor_config.cfgtbl_start;
        box_cfgtbl < (const UvisorBoxConfig**) __uvisor_config.cfgtbl_end;
        box_cfgtbl++
        )
    {
        /* ensure that configuration resides in flash */
        if(!(VMPU_FLASH_ADDR(*box_cfgtbl) &&
            VMPU_FLASH_ADDR(
                ((uint8_t*)(*box_cfgtbl)) + (sizeof(**box_cfgtbl)-1)
            )))
            HALT_ERROR(SANITY_CHECK_FAILED,
                "invalid address - \
                *box_cfgtbl must point to flash (0x%08X)\n", *box_cfgtbl);

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->magic)!=UVISOR_BOX_MAGIC)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid magic\n",
                g_vmpu_box_count,
                (uint32_t)(*box_cfgtbl)
            );

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->version)!=UVISOR_BOX_VERSION)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid version (0x%04X!-0x%04X)\n",
                g_vmpu_box_count,
                *box_cfgtbl,
                (*box_cfgtbl)->version,
                UVISOR_BOX_VERSION
            );

        /* increment box counter */
        if((box_id = g_vmpu_box_count++)>=UVISOR_MAX_BOXES)
            HALT_ERROR(SANITY_CHECK_FAILED, "box number overflow\n");

        /* load box ACLs in table */
        DPRINTF("box[%i] ACL list:\n", box_id);

        /* add ACL's for all box stacks, the actual start addesses and
         * sizes are resolved later in vmpu_initialize_stacks */
        vmpu_acl_stack(
            box_id,
            (*box_cfgtbl)->context_size,
            (*box_cfgtbl)->stack_size
        );

        /* enumerate box ACLs */
        if( (region = (*box_cfgtbl)->acl_list)!=NULL )
        {
            count = (*box_cfgtbl)->acl_count;
            for(i=0; i<count; i++)
            {
                /* ensure that ACL resides in flash */
                if(!VMPU_FLASH_ADDR(region))
                    HALT_ERROR(SANITY_CHECK_FAILED,
                        "box[%i]:acl[%i] must be in code section (@0x%08X)\n",
                        box_id,
                        i,
                        *box_cfgtbl
                    );

                /* add ACL, and force entry as user-provided */
                if(region->acl & UVISOR_TACL_IRQ)
                    vmpu_acl_irq(box_id, region->param1, region->param2);
                else
                    vmpu_acl_add(
                        box_id,
                        region->param1,
                        region->param2,
                        region->acl | UVISOR_TACL_USER
                    );

                /* proceed to next ACL */
                region++;
            }
        }
    }

    /* load box 0 */
    vmpu_load_box(0);

    DPRINTF("vmpu_load_boxes [DONE]\n");
}

/* FIXME: perform ACL checks to close security hole */
int vmpu_fault_recovery_bus(uint32_t pc, uint32_t sp)
{
    uint16_t opcode;
    uint32_t r0, r1;
    uint32_t cnt_max, cnt;
    int found;

    /* check for attacks */
    if(!VMPU_FLASH_ADDR(pc))
       HALT_ERROR(NOT_ALLOWED, "This is not the PC (0x%08X) your were searching for", pc);

    /* if the bus fault is imprecise we will seek back for ldrX/strX opcode */
    if(SCB->CFSR & (1 << 10))
        cnt_max = UVISOR_NOP_CNT;
    else
        cnt_max = 0;

    /* parse opcode */
    cnt = 0;
    do
    {
        /* fetch opcode from memory */
        opcode = vmpu_unpriv_uint16_read(pc - (cnt << 1));

        /* test lower 8bits for (partially)imm5 == 0, Rn = 0, Rt = 1 */
        found = TRUE;
        switch(opcode & 0xFF)
        {
            /* if using r0 and r1, we expect a strX instruction */
            case VMPU_OPCODE16_LOWER_R0_R1_MASK:
                /* fetch r0 and r1 */
                r0 = vmpu_unpriv_uint32_read(sp);
                r1 = vmpu_unpriv_uint32_read(sp+4);

                /* check ACls */
                /* TODO/FIXME */

                /* test upper 8bits for opcode and (partially)imm5 == 0 */
                switch(opcode >> 8)
                {
                    case VMPU_OPCODE16_UPPER_STR_MASK:
                        *((uint32_t *) r0) = (uint32_t) r1;
                        break;
                    case VMPU_OPCODE16_UPPER_STRH_MASK:
                        *((uint16_t *) r0) = (uint16_t) r1;
                        break;
                    case VMPU_OPCODE16_UPPER_STRB_MASK:
                        *((uint8_t *) r0) = (uint8_t) r1;
                        break;
                    default:
                        found = FALSE;
                        break;
                }
                if(found)
                    DPRINTF("Executed privileged access: 0x%08X written to 0x%08X\n\r", r1, r0);
                break;

            /* if using r0 only, we expect a ldrX instruction */
            case VMPU_OPCODE16_LOWER_R0_R0_MASK:
                /* fetch r0 */
                r0 = vmpu_unpriv_uint32_read(sp);

                /* check ACls */
                /* TODO/FIXME */

                /* test upper 8bits for opcode and (partially)imm5 == 0 */
                switch(opcode >> 8)
                {
                    case VMPU_OPCODE16_UPPER_LDR_MASK:
                        r1 = (uint32_t) *((uint32_t *) r0);
                        break;
                    case VMPU_OPCODE16_UPPER_LDRH_MASK:
                        r1 = (uint16_t) *((uint16_t *) r0);
                        break;
                    case VMPU_OPCODE16_UPPER_LDRB_MASK:
                        r1 = (uint8_t) *((uint8_t *) r0);
                        break;
                    default:
                        found = FALSE;
                        break;
                }
                if(found)
                {
                    /* the result is stored back to the stack (r0) */
                    vmpu_unpriv_uint32_write(sp, r1);

                    DPRINTF("Executed privileged access: read 0x%08X from 0x%08X\n\r", r1, r0);
                }
                break;

            default:
                found = FALSE;
                break;
        }

        /* parse next opcode */
        cnt++;
    }
    while(!found && cnt < cnt_max);

    /* return error if opcode was not found */
    if(!found)
        return -1;

    /* otherwise execution continues from the instruction following the fault */
    /* note: we assume the instruction is 16 bits wide and skip possible NOPs */
    vmpu_unpriv_uint32_write(sp + (6 << 2), pc + ((UVISOR_NOP_CNT + 2 - cnt) << 1));

    /* success */
    return 0;
}

int vmpu_init_pre(void)
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
        __uvisor_config.data_start,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );

    /* initialize secured box data sections */
    memcpy(
        __uvisor_config.data_start,
        __uvisor_config.data_src,
        VMPU_REGION_SIZE(__uvisor_config.data_start, __uvisor_config.data_end)
    );

    return vmpu_sanity_checks();
}

void vmpu_init_post(void)
{
    /* enable non-base thread mode */
    /* exceptions can now return to thread mode regardless their origin
     * (supervisor or thread mode); the opposite is not true */
    SCB->CCR |= 1;

    /* init memory protection */
    vmpu_arch_init();

    /* load boxes */
    vmpu_load_boxes();
}
