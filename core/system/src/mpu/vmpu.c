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
#include "debug.h"
#include "context.h"
#include "halt.h"
#include "memory_map.h"
#include "svc.h"
#include "vmpu.h"
#include "vmpu_mpu.h"

#ifndef MPU_MAX_PRIVATE_FUNCTIONS
#define MPU_MAX_PRIVATE_FUNCTIONS 16
#endif /* MPU_MAX_PRIVATE_FUNCTIONS */

#if (MPU_MAX_PRIVATE_FUNCTIONS > 0x100UL)
#error "MPU_MAX_PRIVATE_FUNCTIONS needs to be lower/equal to 0x100"
#endif

uint32_t  g_vmpu_box_count;
bool g_vmpu_boxes_counted;

static int vmpu_sanity_checks(void)
{
    /* Verify the uVisor configuration structure. */
    if (__uvisor_config.magic != UVISOR_MAGIC) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "config magic mismatch: &0x%08X = 0x%08X - exptected 0x%08X\n",
            &__uvisor_config, __uvisor_config.magic, UVISOR_MAGIC);
    }

    /* Verify basic assumptions about vmpu_bits/__builtin_clz. */
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

    /* Verify that the uVisor mode configuration is inside the public flash. */
    assert(vmpu_public_flash_addr((uint32_t) __uvisor_config.mode));
    assert(*(__uvisor_config.mode) <= 2);
    DPRINTF("uVisor mode: %u\n", *(__uvisor_config.mode));

    /* Verify the SRAM relocation. */
    /* Note: SRAM_ORIGIN + SRAM_OFFSET is assumed to be aligned to 32 bytes. */
    assert((uint32_t) __uvisor_config.bss_start == (SRAM_ORIGIN + SRAM_OFFSET));

    DPRINTF("uvisor_ram : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.bss_main_start,
        VMPU_REGION_SIZE(__uvisor_config.bss_main_start, __uvisor_config.bss_main_end));
    DPRINTF("             @0x%08X (%u bytes) [linker]\n",
        SRAM_ORIGIN + SRAM_OFFSET,
        UVISOR_SRAM_LENGTH_USED);

    /* Verify that the sections inside the BSS region are disjoint. */
    DPRINTF("bss_boxes  : @0x%08X (%u bytes) [config]\n",
        __uvisor_config.bss_boxes_start,
        VMPU_REGION_SIZE(__uvisor_config.bss_boxes_start, __uvisor_config.bss_boxes_end));
    assert(__uvisor_config.bss_end > __uvisor_config.bss_start);
    assert(__uvisor_config.bss_main_end > __uvisor_config.bss_main_start);
    assert(__uvisor_config.bss_boxes_end > __uvisor_config.bss_boxes_start);
    assert((__uvisor_config.bss_main_start >= __uvisor_config.bss_boxes_end) ||
           (__uvisor_config.bss_main_end <= __uvisor_config.bss_boxes_start));

    /* Verify the uVisor expectations regarding its own memories. */
    assert(VMPU_REGION_SIZE(__uvisor_config.bss_main_start, __uvisor_config.bss_main_end) == UVISOR_SRAM_LENGTH_USED);
    assert((uint32_t) __uvisor_config.bss_main_end == (SRAM_ORIGIN + SRAM_OFFSET + UVISOR_SRAM_LENGTH_USED));
    assert((uint32_t) __uvisor_config.bss_main_end == (SRAM_ORIGIN + UVISOR_SRAM_LENGTH_PROTECTED));

    /* Verify SRAM sections are within uVisor's own SRAM. */
    assert(&__bss_start__ >= __uvisor_config.bss_main_start);
    assert(&__bss_end__ <= __uvisor_config.bss_main_end);
    assert(&__data_start__ >= __uvisor_config.bss_main_start);
    assert(&__data_end__ <= __uvisor_config.bss_main_end);
    assert(&__stack_start__ >= __uvisor_config.bss_main_start);
    assert(&__stack_end__ <= __uvisor_config.bss_main_end);

    /* Verify that the secure flash area is accessible and after public code. */
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.secure_start));
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.secure_end));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.secure_start));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.secure_end));
    assert(__uvisor_config.secure_start <= __uvisor_config.secure_end);
    assert(__uvisor_config.secure_start >= __uvisor_config.main_end);

    /* Verify the configuration table. */
    assert(__uvisor_config.cfgtbl_ptr_start <= __uvisor_config.cfgtbl_ptr_end);
    assert(__uvisor_config.cfgtbl_ptr_start >= __uvisor_config.secure_start);
    assert(__uvisor_config.cfgtbl_ptr_end <= __uvisor_config.secure_end);
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_start));
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_end));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_start));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.cfgtbl_ptr_end));

    /* Verify the register gateway pointers section. */
    assert(__uvisor_config.register_gateway_ptr_start <= __uvisor_config.register_gateway_ptr_end);
    assert(__uvisor_config.register_gateway_ptr_start >= __uvisor_config.secure_start);
    assert(__uvisor_config.register_gateway_ptr_end <= __uvisor_config.secure_end);
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.register_gateway_ptr_start));
    assert(!vmpu_public_flash_addr((uint32_t) __uvisor_config.register_gateway_ptr_end));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.register_gateway_ptr_start));
    assert(vmpu_flash_addr((uint32_t) __uvisor_config.register_gateway_ptr_end));

    /* Verify that every register gateway in memory is aligned to 4 bytes. */
    uint32_t * register_gateway = __uvisor_config.register_gateway_ptr_start;
    for (; register_gateway < __uvisor_config.register_gateway_ptr_end; register_gateway++) {
        if (*register_gateway & 0x3) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Register gateway 0x%08X is not aligned to 4 bytes",
                (uint32_t) register_gateway);
        }
    }

    /* Return an error if uVisor is disabled. */
    if (!__uvisor_config.mode || (*__uvisor_config.mode == 0)) {
        return -1;
    } else {
        return 0;
    }
}

static void vmpu_sanity_check_box_namespace(int box_id, const char *const box_namespace)
{
    /* Verify that all characters of the box_namespace (including the trailing
     * NUL) are within flash and that the box_namespace is not too long. It is
     * also okay for the box namespace to be NULL. */
    size_t length = 0;

    if (box_namespace == NULL) {
        return;
    }

    do {
        /* Check that the address of the character is within public flash before
         * reading the character. */
        /* Note: The public flash section is assumed to be monolithic, so if
         * both the start and end address of an array are inside the public
         * flash, then the whole array is inside the public flash. */
        if (!vmpu_public_flash_addr((uint32_t) &box_namespace[0]) ||
            !vmpu_public_flash_addr((uint32_t) &box_namespace[length])) {
            HALT_ERROR(SANITY_CHECK_FAILED, "box[%i] @0x%08X - namespace not entirely in public flash\n",
                box_id, box_namespace, UVISOR_MAX_BOX_NAMESPACE_LENGTH);
        }

        if (box_namespace[length] == '\0') {
            /* If we reached the end of the string, which we now know is stored
             * in flash, then we are done. */
            break;
        }

        ++length;

        if (length >= UVISOR_MAX_BOX_NAMESPACE_LENGTH) {
            HALT_ERROR(SANITY_CHECK_FAILED,
                "box[%i] @0x%08X - namespace too long (length >= %u)\n",
                box_id, box_namespace, UVISOR_MAX_BOX_NAMESPACE_LENGTH);
        }
    } while (box_namespace[length]);
}

static void vmpu_box_index_init(uint8_t box_id, const UvisorBoxConfig * const config)
{
    uint8_t * box_bss;
    UvisorBoxIndex * index;
    uint32_t heap_size = config->heap_size;
    int i;

    if (box_id == 0) {
        /* Box 0 still uses the main heap to be backwards compatible. */
        const uint32_t heap_end = (uint32_t) __uvisor_config.heap_end;
        const uint32_t heap_start = (uint32_t) __uvisor_config.heap_start;
        heap_size = (heap_end - heap_start) - config->index_size;
    }

    box_bss = (uint8_t *) g_context_current_states[box_id].bss;

    /* The box index is at the beginning of the bss section. */
    index = (void *) box_bss;
    /* Zero the _entire_ index, so that user data inside the box index is in a
     * known state! This allows checking variables for `NULL`, or `0`, which
     * indicates an initialization requirement. */
    memset(index, 0, config->index_size);
    box_bss += config->index_size;

    for (i = 0; i < UVISOR_BOX_INDEX_SIZE_COUNT; i++) {
        index->bss_ptr[i] = (void *) (config->bss_size[i] ? box_bss : NULL);
        box_bss += config->bss_size[i];
        if (box_id == 0) {
            heap_size -= config->bss_size[i];
        }
    }

    /* Initialize box heap. */
    index->box_heap = (void *) (heap_size ? box_bss : NULL);
    index->box_heap_size = heap_size;

    /* Active heap pointer is NULL, indicating that the process heap needs to
     * be initialized on first malloc call! */
    index->active_heap = NULL;

    /* Point to the box config. */
    index->config = config;
}

static void vmpu_load_boxes(void)
{
    int i, count;
    const UvisorBoxAclItem *region;
    const UvisorBoxConfig **box_cfgtbl;
    uint32_t bss_size;
    uint8_t box_id;

    /* Check heap start and end addresses. */
    if (!__uvisor_config.heap_start || !vmpu_sram_addr((uint32_t) __uvisor_config.heap_start)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap start pointer (0x%08x) is not in SRAM memory.\n",
            (uint32_t) __uvisor_config.heap_start);
    }
    if (!__uvisor_config.heap_end || !vmpu_sram_addr((uint32_t) __uvisor_config.heap_end)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap end pointer (0x%08x) is not in SRAM memory.\n",
            (uint32_t) __uvisor_config.heap_end);
    }
    if (__uvisor_config.heap_end < __uvisor_config.heap_start) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap end pointer (0x%08x) is smaller than heap start pointer (0x%08x).\n",
            (uint32_t) __uvisor_config.heap_end, (uint32_t) __uvisor_config.heap_start);
    }

    /* Enumerate boxes. */
    g_vmpu_box_count = (uint32_t) (__uvisor_config.cfgtbl_ptr_end - __uvisor_config.cfgtbl_ptr_start);
    if (g_vmpu_box_count >= UVISOR_MAX_BOXES) {
        HALT_ERROR(SANITY_CHECK_FAILED, "box number overflow\n");
    }
    g_vmpu_boxes_counted = TRUE;

    /* Initialize boxes. */
    box_id = 0;
    for (box_cfgtbl = (const UvisorBoxConfig * *) __uvisor_config.cfgtbl_ptr_start;
         box_cfgtbl < (const UvisorBoxConfig * *) __uvisor_config.cfgtbl_ptr_end;
         box_cfgtbl++) {
        /* Ensure that the configuration table resides in flash. */
        if (!(vmpu_flash_addr((uint32_t) *box_cfgtbl) &&
            vmpu_flash_addr((uint32_t) ((uint8_t *) (*box_cfgtbl)) + (sizeof(**box_cfgtbl) - 1)))) {
            HALT_ERROR(SANITY_CHECK_FAILED, "invalid address - *box_cfgtbl must point to flash (0x%08X)\n",
                *box_cfgtbl);
        }

        /* Check the magic value in the box configuration table. */
        if (((*box_cfgtbl)->magic) != UVISOR_BOX_MAGIC) {
            HALT_ERROR(SANITY_CHECK_FAILED, "box[%i] @0x%08X - invalid magic\n",
                box_id, (uint32_t)(*box_cfgtbl));
        }

        /* Check the box configuration table version. */
        if (((*box_cfgtbl)->version) != UVISOR_BOX_VERSION) {
            HALT_ERROR(SANITY_CHECK_FAILED, "box[%i] @0x%08X - invalid version (0x%04X!-0x%04X)\n",
                box_id, *box_cfgtbl, (*box_cfgtbl)->version, UVISOR_BOX_VERSION);
        }

        /* Confirm the minimal size of the box index size. */
        if ((*box_cfgtbl)->index_size < sizeof(UvisorBoxIndex)) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Box index size (%uB) must be large enough to hold UvisorBoxIndex (%uB).\n",
                (*box_cfgtbl)->index_size, sizeof(UvisorBoxIndex));
        }

        /* Check that the box namespace is not too long. */
        vmpu_sanity_check_box_namespace(box_id, (*box_cfgtbl)->box_namespace);

        /* Load the box ACLs. */
        DPRINTF("box[%i] ACL list:\n", box_id);

        /* Add ACL's for all box stacks. */
        bss_size = (*box_cfgtbl)->index_size + (*box_cfgtbl)->heap_size;
        for (i = 0; i < UVISOR_BOX_INDEX_SIZE_COUNT; i++) {
            bss_size += (*box_cfgtbl)->bss_size[i];
        }
        vmpu_acl_stack(
            box_id,
            bss_size,
            (*box_cfgtbl)->stack_size
        );

        /* Initialize box index. */
        vmpu_box_index_init(
            box_id,
            *box_cfgtbl
        );

        /* Enumerate the box ACLs. */
        region = (*box_cfgtbl)->acl_list;
        if (region != NULL) {
            count = (*box_cfgtbl)->acl_count;
            for (i = 0; i < count; i++) {
                /* Ensure that the ACL resides in public flash. */
                if (!vmpu_public_flash_addr((uint32_t) region)) {
                    HALT_ERROR(SANITY_CHECK_FAILED, "box[%i]:acl[%i] must be in code section (@0x%08X)\n",
                        box_id, i, *box_cfgtbl);
                }

                /* Add the ACL and force the entry as user-provided. */
                if (region->acl & UVISOR_TACL_IRQ) {
                    vmpu_acl_irq(box_id, region->param1, region->param2);
                } else {
                    vmpu_region_add_static_acl(
                        box_id,
                        (uint32_t) region->param1,
                        region->param2,
                        region->acl | UVISOR_TACL_USER,
                        0
                    );
                }

                /* Proceed to the next ACL. */
                region++;
            }
        }

        /* Proceed to the next box. */
        box_id++;
    }

    /* Load box 0. */
    vmpu_load_box(0);
    *(__uvisor_config.uvisor_box_context) = (uint32_t *) g_context_current_states[0].bss;

    DPRINTF("vmpu_load_boxes [DONE]\n");
}

int vmpu_fault_recovery_bus(uint32_t pc, uint32_t sp, uint32_t fault_addr, uint32_t fault_status)
{
    uint16_t opcode;
    uint32_t r0, r1;
    uint32_t cnt_max, cnt;
    int found;

    /* Check for attacks. */
    if (!vmpu_public_flash_addr(pc)) {
       HALT_ERROR(NOT_ALLOWED, "This is not the PC (0x%08X) your were searching for", pc);
    }

    /* Check fault register; the following two configurations are allowed:
     *   0x04 - imprecise data bus fault, no stacking/unstacking errors.
     *   0x82 - precise data bus fault, no stacking/unstacking errors. */
    /* Note: Currently the faulting address argument is not used, since it
     * is saved in r0 for managed bus faults. */
    switch (fault_status) {
        case 0x82:
            cnt_max = 0;
            break;
        case 0x04:
            cnt_max = UVISOR_NOP_CNT;
            break;
        default:
            return -1;
    }

    /* Parse the instruction opcode. */
    cnt = 0;
    do {
        /* Fetch the opcode from memory. */
        opcode = vmpu_unpriv_uint16_read(pc - (cnt << 1));

        /* Test the lower 8 bits for imm5 = 0, Rn = 0, Rt = 1. */
        found = TRUE;
        switch(opcode & 0xFF) {
            /* If using r0 and r1, we expect a strX instruction. */
            case VMPU_OPCODE16_LOWER_R0_R1_MASK:
                /* Fetch r0 and r1. */
                r0 = vmpu_unpriv_uint32_read(sp);
                r1 = vmpu_unpriv_uint32_read(sp+4);

                /* Check if there is an ACL mapping this access. */
                if ((vmpu_fault_find_acl(r0, sizeof(uint32_t)) & UVISOR_TACL_UWRITE) == 0) {
                    return -1;
                };

                /* Test the upper 8 bits for the desired opcode and imm5 = 0. */
                switch (opcode >> 8) {
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

            /* If using r0 only, we expect a ldrX instruction. */
            case VMPU_OPCODE16_LOWER_R0_R0_MASK:
                /* Fetch r0. */
                r0 = vmpu_unpriv_uint32_read(sp);

                /* Check if there is an ACL mapping this access. */
                if ((vmpu_fault_find_acl(r0, sizeof(uint32_t)) & UVISOR_TACL_UREAD) == 0) {
                    return -1;
                };

                /* Test the upper 8 bits for the desired opcode and imm5 = 0. */
                switch (opcode >> 8) {
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
                if (found) {
                    /* The result is stored back to the stack (r0). */
                    vmpu_unpriv_uint32_write(sp, r1);
                    /* DPRINTF("Executed privileged access: read 0x%08X from 0x%08X\n\r", r1, r0); */
                }
                break;

            default:
                found = FALSE;
                break;
        }

        /* Parse the next opcode. */
        cnt++;
    } while (!found && cnt < cnt_max);

    /* Return an error if the opcode was not found. */
    if (!found) {
        return -1;
    }

    /* Otherwise execution continues from the instruction following the fault. */
    /* Note: We assume the instruction is 16 bits wide and skip possible NOPs. */
    vmpu_unpriv_uint32_write(sp + (6 << 2), pc + ((UVISOR_NOP_CNT + 2 - cnt) << 1));

    /* Success. */
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

int vmpu_box_id_caller(void)
{
    TContextPreviousState * previous_state;

    previous_state = context_state_previous();
    if (previous_state == NULL) {
        /* There is no previous context. */
        return -1;
    }

    if (previous_state->type != CONTEXT_SWITCH_FUNCTION_GATEWAY) {
        /* The previous context is not a secure gateway. */
        return -1;
    } else {
        return previous_state->src_id;
    }
}

static int copy_box_namespace(const char *src, char *dst)
{
    int bytes_copied;

    /* Copy the box namespace to the client-provided destination. */
    for (bytes_copied = 0; bytes_copied < UVISOR_MAX_BOX_NAMESPACE_LENGTH; bytes_copied++) {
        vmpu_unpriv_uint8_write((uint32_t)&dst[bytes_copied], src[bytes_copied]);

        if (src[bytes_copied] == '\0') {
            /* We've reached the end of the box namespace. */
            ++bytes_copied; /* Include the terminating-null in bytes_copied. */
            goto done;
        }
    }

    /* We did not find a terminating null in the src. The src has been verified
     * in vmpu_box_namespace_from_id as being in the box config table. It is a
     * programmer error if the namespace in the box config table is not
     * null-terminated, so we halt. */
    HALT_ERROR(SANITY_CHECK_FAILED, "vmpu: Box namespace missing terminating-null\r\n");

done:
    return bytes_copied;
}

int vmpu_box_namespace_from_id(int box_id, char *box_namespace, size_t length)
{
    const UvisorBoxConfig **box_cfgtbl;
    box_cfgtbl = (const UvisorBoxConfig**) __uvisor_config.cfgtbl_ptr_start;

    if (!vmpu_is_box_id_valid(box_id)) {
        /* The box_id is not valid, so return an error to prevent reading
         * non-box-configuration data from flash. */
        return UVISOR_ERROR_INVALID_BOX_ID;
    }

    if (length < UVISOR_MAX_BOX_NAMESPACE_LENGTH) {
        /* There is not enough room in the supplied buffer for maximum length
         * namespace. */
       return UVISOR_ERROR_BUFFER_TOO_SMALL;
    }

    if (box_cfgtbl[box_id]->box_namespace == NULL) {
        return UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS;
    }

    return copy_box_namespace(box_cfgtbl[box_id]->box_namespace, box_namespace);
}
