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
#ifdef SRAM_OFFSET
#  define SRAM_OFFSET_START UVISOR_REGION_ROUND_UP(SRAM_ORIGIN + SRAM_OFFSET)
#else
#  define SRAM_OFFSET_START SRAM_ORIGIN
#endif

#if (MPU_MAX_PRIVATE_FUNCTIONS>0x100UL)
#error "MPU_MAX_PRIVATE_FUNCTIONS needs to be lower/equal to 0x100"
#endif

uint32_t  g_vmpu_box_count;
uint8_t g_active_box;

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

    /* Verify that the core version is the same as expected. */
    if (!CORE_VERSION_CHECK() || !CORE_REVISION_CHECK()) {
        HALT_ERROR(SANITY_CHECK_FAILED, "This core is unsupported or there is a mismatch between the uVisor "
                                        "configuration you are using and the core this configuration supports.\n\r");
    }

    /* Verify that the known hard-coded symbols are equal to the ones taken from
     * the host linker script. */
    assert((uint32_t) __uvisor_config.flash_start == FLASH_ORIGIN);
    assert((uint32_t) __uvisor_config.sram_start == SRAM_ORIGIN);

    /* Verify that the uVisor binary blob is positioned at the flash offset. */
    assert(((uint32_t) __uvisor_config.flash_start + FLASH_OFFSET) == (uint32_t) __uvisor_config.main_start);

    /* verify if configuration mode is inside flash memory */
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.mode));
    assert(*(__uvisor_config.mode) <= 2);
    DPRINTF("uVisor mode: %u\n", *(__uvisor_config.mode));

    /* verify SRAM relocation */
    DPRINTF("uvisor_ram : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.bss_main_start,
        VMPU_REGION_SIZE(__uvisor_config.bss_main_start,
                         __uvisor_config.bss_main_end));
    DPRINTF("             (0x%08X (%u bytes) [linker]\n",
            SRAM_OFFSET_START, UVISOR_SRAM_LENGTH);
    assert( __uvisor_config.bss_main_end > __uvisor_config.bss_main_start );
    assert( VMPU_REGION_SIZE(__uvisor_config.bss_main_start,
                             __uvisor_config.bss_main_end) == UVISOR_SRAM_LENGTH );
    assert(&__stack_end__ <= __uvisor_config.bss_main_end);

    assert( (uint32_t) __uvisor_config.bss_main_start == SRAM_OFFSET_START);
    assert( (uint32_t) __uvisor_config.bss_main_end == (SRAM_OFFSET_START +
                                                        UVISOR_SRAM_LENGTH) );

    /* verify that secure flash area is accessible and after public code */
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.secure_start));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.secure_end));
    assert(__uvisor_config.secure_start <= __uvisor_config.secure_end);
    assert(__uvisor_config.secure_start >= __uvisor_config.main_end);

    /* verify configuration table */
    assert(__uvisor_config.cfgtbl_ptr_start <= __uvisor_config.cfgtbl_ptr_end);
    assert(__uvisor_config.cfgtbl_ptr_start >= __uvisor_config.secure_start);
    assert(__uvisor_config.cfgtbl_ptr_end <= __uvisor_config.secure_end);
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_start));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_end));

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

    /* enumerate boxes */
    g_vmpu_box_count = (uint32_t) (__uvisor_config.cfgtbl_ptr_end - __uvisor_config.cfgtbl_ptr_start);
    if (g_vmpu_box_count >= UVISOR_MAX_BOXES) {
        HALT_ERROR(SANITY_CHECK_FAILED, "box number overflow\n");
    }

    /* initialize boxes */
    box_id = 0;
    for(box_cfgtbl = (const UvisorBoxConfig**) __uvisor_config.cfgtbl_ptr_start;
        box_cfgtbl < (const UvisorBoxConfig**) __uvisor_config.cfgtbl_ptr_end;
        box_cfgtbl++
        )
    {
        /* ensure that configuration resides in flash */
        if(!(vmpu_flash_addr((uint32_t) *box_cfgtbl) &&
            vmpu_flash_addr((uint32_t) ((uint8_t*)(*box_cfgtbl)) + (sizeof(**box_cfgtbl)-1))))
            HALT_ERROR(SANITY_CHECK_FAILED,
                "invalid address - \
                *box_cfgtbl must point to flash (0x%08X)\n", *box_cfgtbl);

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->magic)!=UVISOR_BOX_MAGIC)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid magic\n",
                box_id,
                (uint32_t)(*box_cfgtbl)
            );

        /* check for magic value in box configuration */
        if(((*box_cfgtbl)->version)!=UVISOR_BOX_VERSION)
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - invalid version (0x%04X!-0x%04X)\n",
                box_id,
                *box_cfgtbl,
                (*box_cfgtbl)->version,
                UVISOR_BOX_VERSION
            );

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
                if(!vmpu_flash_addr((uint32_t) region))
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

        box_id++;
    }

    /* load box 0 */
    vmpu_load_box(0);

    DPRINTF("vmpu_load_boxes [DONE]\n");
}

uint32_t vmpu_register_gateway(uint32_t addr, uint32_t val)
{
    return 0;
}

int vmpu_fault_recovery_bus(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status)
{
    uint16_t opcode;
    uint32_t r0, r1;
    uint32_t cnt_max, cnt;
    int found;

    /* check for attacks */
    if(!vmpu_flash_addr(pc))
       HALT_ERROR(NOT_ALLOWED, "This is not the PC (0x%08X) your were searching for", pc);

    /* check fault register; the following two configurations are allowed:
     *   0x04 - imprecise data bus fault, no stacking/unstacking errors
     *   0x82 - precise data bus fault, no stacking/unstacking errors */
    /* note: currently the faulting address argument is not used, since it
     * is saved in r0 for managed bus faults */
    switch(fault_status) {
        case 0x82:
            cnt_max = 0;
            break;
        case 0x04:
            cnt_max = UVISOR_NOP_CNT;
            break;
        default:
            return -1;
    }

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
                if((vmpu_fault_find_acl(r0,sizeof(uint32_t)) & UVISOR_TACL_UWRITE) == 0) {
                    return -1;
                };

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
                if (found) {
                    /* DPRINTF("Executed privileged access: 0x%08X written to 0x%08X\n\r", r1, r0); */
                }
                break;

            /* if using r0 only, we expect a ldrX instruction */
            case VMPU_OPCODE16_LOWER_R0_R0_MASK:
                /* fetch r0 */
                r0 = vmpu_unpriv_uint32_read(sp);

                /* check ACls */
                if((vmpu_fault_find_acl(r0,sizeof(uint32_t)) & UVISOR_TACL_UREAD) == 0) {
                    return -1;
                };

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

                    /* DPRINTF("Executed privileged access: read 0x%08X from 0x%08X\n\r", r1, r0); */
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
    return vmpu_sanity_checks();
}

void vmpu_init_post(void)
{
    /* Enable non-base thread mode (NONBASETHRDENA).
     * Exceptions can now return to thread mode regardless their origin
     * (supervisor or thread mode); the opposite is not true. */
    SCB->CCR |= 1;

    /* init memory protection */
    vmpu_arch_init();

    /* load boxes */
    vmpu_load_boxes();
}

int vmpu_box_id_self(void)
{
    return g_active_box;
}
