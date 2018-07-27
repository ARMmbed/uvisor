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
#include "api/inc/box_config.h"
#include "api/inc/debug_exports.h"
#include "api/inc/uvisor_exports.h"
#include "box_init.h"
#include "debug.h"
#include "context.h"
#include "halt.h"
#include "svc.h"
#include "virq.h"
#include "vmpu.h"
#include "vmpu_mpu.h"
#include "vmpu_unpriv_access.h"
#include <sys/reent.h>


extern uint32_t g_debug_interrupt_sp[UVISOR_MAX_BOXES];
uint8_t g_vmpu_box_count;
bool g_vmpu_boxes_counted;

static int vmpu_check_sanity(void)
{
    /* Verify the uVisor configuration structure. */
    if (__uvisor_config.magic != UVISOR_MAGIC) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "config magic mismatch: &0x%08X = 0x%08X - exptected 0x%08X\n",
            &__uvisor_config, __uvisor_config.magic, UVISOR_MAGIC);
    }

    /* Make sure the UVISOR_BOX_ID_INVALID is definitely NOT a valid box id. */
    assert(!vmpu_is_box_id_valid(UVISOR_BOX_ID_INVALID));

    /* Verify basic assumptions about vmpu_bits/__builtin_clz. */
    assert(__builtin_clz(1UL << 31) == 0);
    assert(vmpu_bits(0) == 0);
    assert(vmpu_bits(1UL << 31) == 32);
    assert(vmpu_bits(0x8000UL) == 16);
    assert(vmpu_bits(0x8001UL) == 16);
    assert(vmpu_bits(1) == 1);

    /* Verify that the core version is the same as expected. */
    if (!CORE_VERSION_CHECK() || !CORE_REVISION_CHECK()) {
        HALT_ERROR(SANITY_CHECK_FAILED, "This core is unsupported or there is a mismatch between the uVisor "
                                        "configuration you are using and the core this configuration supports.\n");
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

    /* Verify the position of the page heap section. */
    assert(__uvisor_config.page_start >= __uvisor_config.public_sram_start);
    assert(__uvisor_config.page_start <= __uvisor_config.page_end);
    if (__uvisor_config.public_sram_start == __uvisor_config.sram_start) {
        /* If uVisor shares the SRAM with the OS/app, "SRAM" and "public SRAM"
         * must be the same thing for uVisor. */
        assert(__uvisor_config.page_start >= __uvisor_config.bss_end);
        assert(__uvisor_config.public_sram_end == __uvisor_config.sram_end);
    } else {
        /* If uVisor uses a separate memory (e.g. a TCM), "SRAM" and "public
         * SRAM" must be disjoint. */
        assert((__uvisor_config.public_sram_start >= __uvisor_config.sram_end) ||
               (__uvisor_config.sram_start >= __uvisor_config.public_sram_end));
    }

    /* Verify SRAM sections are within uVisor's own SRAM. */
    assert(&__uvisor_bss_start__ >= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_start));
    assert(&__uvisor_bss_end__ <= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_end));
    assert(&__uvisor_data_start__ >= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_start));
    assert(&__uvisor_data_end__ <= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_end));
    assert(&__uvisor_stack_start__ >= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_start));
    assert(&__uvisor_stack_end__ <= UVISOR_GET_S_ALIAS(__uvisor_config.bss_main_end));

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

    /* Check the public heap start and end addresses. */
    uint32_t const heap_end = (uint32_t) __uvisor_config.heap_end;
    uint32_t const heap_start = (uint32_t) __uvisor_config.heap_start;
    if (!heap_start || !vmpu_public_sram_addr(heap_start)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap start pointer (0x%08x) is not in SRAM memory.\n", heap_start);
    }
    if (!heap_end || !vmpu_public_sram_addr(heap_end)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap end pointer (0x%08x) is not in SRAM memory.\n", heap_end);
    }
    if (heap_end < heap_start) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Heap end pointer (0x%08x) is smaller than heap start pointer (0x%08x).\n",
                   heap_end, heap_start);
    }

    /* Return an error if uVisor is disabled. */
    if (!__uvisor_config.mode || (*__uvisor_config.mode == 0)) {
        return -1;
    } else {
        return 0;
    }
}

static void vmpu_check_sanity_box_namespace(int box_id, const char *const box_namespace)
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

static void vmpu_check_sanity_box_cfgtbl(uint8_t box_id, UvisorBoxConfig const * box_cfgtbl)
{
    /* Ensure that the configuration table resides in flash. */
    if (!(vmpu_flash_addr((uint32_t) box_cfgtbl) &&
        vmpu_flash_addr((uint32_t) ((uint8_t *) box_cfgtbl + (sizeof(*box_cfgtbl) - 1))))) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: The box configuration table is not in flash.\n",
                   box_id, (uint32_t) box_cfgtbl);
    }

    /* Check the magic value in the box configuration table. */
    if (box_cfgtbl->magic != UVISOR_BOX_MAGIC) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Invalid magic number (found: 0x%08X, exepcted 0x%08X).\n",
                   box_id, (uint32_t) box_cfgtbl, box_cfgtbl->magic, UVISOR_BOX_MAGIC);
    }

    /* Check the box configuration table version. */
    if (box_cfgtbl->version != UVISOR_BOX_VERSION) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Invalid version number (found: 0x%04X, expected: 0x%04X).\n",
                   box_id, (uint32_t) box_cfgtbl, box_cfgtbl->version, UVISOR_BOX_VERSION);
    }

    /* Check the minimal size of the box index size. */
    if (box_cfgtbl->bss.size_of.index < sizeof(UvisorBoxIndex)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Index size (%uB) < sizeof(UvisorBoxIndex) (%uB).\n",
                   box_id, (uint32_t) box_cfgtbl, box_cfgtbl->bss.size_of.index, sizeof(UvisorBoxIndex));
    }

    /* Check the minimal size of the box stack. */
    if (box_id != 0 && (box_cfgtbl->stack_size < UVISOR_MIN_STACK_SIZE)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Stack size (%uB) < UVISOR_MIN_STACK_SIZE (%uB).\n",
                   box_id, (uint32_t) box_cfgtbl, box_cfgtbl->stack_size, UVISOR_MIN_STACK_SIZE);
    }

    /* Checks specific to the public box */
    if (box_id == 0) {
        /* We require both the stack and the heap to be set to 0. If the user
         * configures the public box via our macros it is always the case. */
        /* FIXME: At the moment we cannot calculate the stack and heap available
         *        to the public box at compile/link time, so we have to set them
         *        to zero and then determine them at runtime (see code in box
         *        index initialization). We should remove this dependency. */
        if (box_cfgtbl->stack_size != 0) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: The public box stack size must be set to 0 (found %uB).\n",
                       box_id, (uint32_t) box_cfgtbl, box_cfgtbl->stack_size);
        }
        if (box_cfgtbl->bss.size_of.heap != 0) {
            HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: The public box heap size must be set to 0 (found %uB).\n",
                       box_id, (uint32_t) box_cfgtbl, box_cfgtbl->bss.size_of.heap);
        }
    }

    /* Check that the box namespace is not too long. */
    vmpu_check_sanity_box_namespace(box_id, box_cfgtbl->box_namespace);
}

static void vmpu_box_index_init(uint8_t box_id, UvisorBoxConfig const * const box_cfgtbl, void * const bss_start)
{
    /* The box index is at the beginning of the BSS section. */
    void * box_bss = bss_start;
    UvisorBoxIndex * index = (UvisorBoxIndex *) box_bss;

    /* Assign the pointers to the BSS sections. */
    for (int i = 0; i < UVISOR_BSS_SECTIONS_COUNT; i++) {
        /* Round size up to a multiple of 4. */
        size_t size = __UVISOR_BOX_ROUND_4(box_cfgtbl->bss.sizes[i]);
        index->bss.pointers[i] = (size ? box_bss : NULL);
        box_bss += size;
    }
    size_t bss_size = box_bss - bss_start;

    /* Initialize the box heap size. */
    uint32_t heap_size = 0;
    if (box_id == 0) {
        /* FIXME: At the moment we cannot just reliably use the size of the heap
         *        as provided by the configuration table, because the public box
         *        has a variable heap size that is not identified at compile
         *        time. We should remove this dependency. */
        const uint32_t heap_end = (uint32_t) __uvisor_config.heap_end;
        const uint32_t heap_start = (uint32_t) __uvisor_config.heap_start;
        heap_size = heap_end - heap_start - bss_size;

        /* Set the heap pointer for the public box.
         * This value must be set manually because the previous loop will have
         * caught a zero heap size and thus set the pointer to NULL. */
        index->bss.address_of.heap = (uint32_t) ((void *) heap_start + bss_size);
    } else {
        heap_size = box_cfgtbl->bss.size_of.heap;

        /* TODO: Move this into box_init on NS side. */
        /* The _REENT_INIT_PTR points these buffers to other parts of the struct,
         * which, since this is executed in S-mode, aliases to the secure addresses.
         * That has to be corrected. The rest of the reent struct is initialized to
         * zero by _REENT_INIT_PTR, but this is newlib implementation defined. */
        struct _reent * reent = (struct _reent *) index->bss.address_of.newlib_reent;
        _REENT_INIT_PTR(UVISOR_GET_S_ALIAS(reent));
        UVISOR_GET_S_ALIAS(reent)->_stdin  = UVISOR_GET_NS_ALIAS(UVISOR_GET_S_ALIAS(reent)->_stdin);
        UVISOR_GET_S_ALIAS(reent)->_stdout = UVISOR_GET_NS_ALIAS(UVISOR_GET_S_ALIAS(reent)->_stdout);
        UVISOR_GET_S_ALIAS(reent)->_stderr = UVISOR_GET_NS_ALIAS(UVISOR_GET_S_ALIAS(reent)->_stderr);
    }
    index->box_heap_size = heap_size;

    /* Active heap pointer is NULL, indicating that the process heap needs to
     * be initialized on first malloc call! */
    index->active_heap = NULL;

    /* Point to the box config. */
    index->config = box_cfgtbl;
}

static void vmpu_configure_box_peripherals(uint8_t box_id, UvisorBoxConfig const * const box_cfgtbl)
{
    /* Enumerate the box ACLs. */
    const UvisorBoxAclItem * region = box_cfgtbl->acl_list;
    if (region != NULL) {
        int count = box_cfgtbl->acl_count;
        for (int i = 0; i < count; i++) {
            /* Ensure that the ACL resides in public flash. */
            if (!vmpu_public_flash_addr((uint32_t) region)) {
                HALT_ERROR(SANITY_CHECK_FAILED, "box[%i]:acl[%i] must be in code section (@0x%08X)i\n",
                           box_id, i, box_cfgtbl);
            }

            /* Add the ACL and force the entry as user-provided. */
            if (region->acl & UVISOR_TACL_IRQ) {
                virq_acl_add(box_id, (uint32_t) region->param1);
            } else {
                vmpu_region_add_static_acl(
                    box_id,
                    (uint32_t) region->param1,
                    region->param2,
                    region->acl | UVISOR_TACL_USER,
                    0
                );
                DPRINTF("  - Peripheral: 0x%08X - 0x%08X (permissions: 0x%04X)\n",
                        (uint32_t) region->param1, (uint32_t) region->param1 + region->param2,
                        region->acl | UVISOR_TACL_USER);
            }

            /* Proceed to the next ACL. */
            region++;
        }
    }
}

static void vmpu_configure_box_sram(uint8_t box_id, UvisorBoxConfig const * box_cfgtbl)
{
    /* Compute the size of the BSS sections. */
    uint32_t bss_size = 0;
    for (int i = 0; i < UVISOR_BSS_SECTIONS_COUNT; i++) {
        bss_size += box_cfgtbl->bss.sizes[i];
    }

    /* Add ACLs for the secure box BSS sections and for the stack. */
    uint32_t bss_start = 0;
    uint32_t stack_pointer = 0;
    if (box_id == 0) {
        /* The public box does not need an explicit ACL. Instead, it uses the
         * default access permissions given by the background regions. */
        /* Note: This relies on the assumption that all supported MPU
         *       architectures allow us to setup a background region somehow. */
        bss_start = (uint32_t) __uvisor_config.heap_start;
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
        stack_pointer = __TZ_get_SP_NS();
#else
        stack_pointer = __get_PSP();
#endif
    } else {
        uint32_t stack_size = box_cfgtbl->stack_size;
        vmpu_acl_sram(box_id, bss_size, stack_size, &bss_start, &stack_pointer);
    }

    /* Set the box state for the SRAM sections. */
    g_context_current_states[box_id].bss = bss_start;
    g_context_current_states[box_id].bss_size = bss_size;
    g_context_current_states[box_id].sp = stack_pointer;

    /* Initialize the box index. */
    memset((void *) bss_start, 0, bss_size);
    vmpu_box_index_init(box_id, box_cfgtbl, (void *) bss_start);
}

static void vmpu_enumerate_boxes(void)
{
    bool debug_box_validated = false;

    /* Enumerate boxes. */
    g_vmpu_box_count = (uint32_t) (__uvisor_config.cfgtbl_ptr_end - __uvisor_config.cfgtbl_ptr_start);
    if (g_vmpu_box_count >= UVISOR_MAX_BOXES) {
        HALT_ERROR(SANITY_CHECK_FAILED, "box number overflow\n");
    }
    g_vmpu_boxes_counted = TRUE;

    /* Get the boxes order. This is MPU-specific. */
    int box_order[UVISOR_MAX_BOXES] = {0};
    vmpu_order_boxes(box_order, g_vmpu_box_count);

    /* Initialize the boxes. */
    for (uint8_t box_id = 0; box_id < g_vmpu_box_count; ++box_id) {
        /* Select the pointer to the (permuted) box configuration table. */
        int index = box_order[box_id];
        UvisorBoxConfig const * box_cfgtbl = ((UvisorBoxConfig const * *) __uvisor_config.cfgtbl_ptr_start)[index];
        TUvisorDebugDriver const * const debug_driver = __uvisor_config.debug_driver;

        /* debug_driver should never be NULL */
        assert(debug_driver);

        /* If a debug driver was created by the application and it is associated to this box_id,
         * initialize g_debug_box accordingly.
         * Note: The verification that no more than one box was created and been associated with the debug driver
         * is done statically on debug driver creation time [in build time] */
        if (debug_driver->box_cfg_ptr == box_cfgtbl) {

            // No more than one box should be created and associated with the debug driver (see note above)
            assert(!debug_box_validated);

            /* Before g_debug_box initialization, verify magic and version of the debug box. */
            if (debug_driver->magic != UVISOR_DEBUG_BOX_MAGIC) {
                HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Wrong debug box magic.\n",
                        box_id, (uint32_t) box_cfgtbl);
            }
            if (debug_driver->version != UVISOR_DEBUG_BOX_VERSION) {
                HALT_ERROR(SANITY_CHECK_FAILED, "Box %i @0x%08X: Wrong debug box version.\n",
                        box_id, (uint32_t) box_cfgtbl);
            }

            g_debug_box.driver = debug_driver;
            g_debug_box.box_id = box_id;

            /* Set g_debug_box.initialized only after g_debug_interrupt_sp[] is set */
            debug_box_validated = true;
        }

        /* Verify the box configuration table. */
        /* Note: This function halts if a sanity check fails. */
        vmpu_check_sanity_box_cfgtbl(index, box_cfgtbl);

        DPRINTF("Box %i ACLs:\n", index);

        /* Add the box ACL for the static SRAM memories. */
        vmpu_configure_box_sram(index, box_cfgtbl);

        /* Add the box ACLs for peripherals. */
        /* MUST call this function with the new indexing since it is a stateful */
        /*   as it is increments g_mpu_region in call order */
        vmpu_configure_box_peripherals(index, box_cfgtbl);

        box_init(index, box_cfgtbl);
    }

    if (debug_box_validated) {
        for (int ii = 0; ii < UVISOR_MAX_BOXES; ii++) {
            g_debug_interrupt_sp[ii] = g_context_current_states[ii].sp;
        }
        g_debug_box.initialized = 1;
    }

    /* Load box 0. */
    context_switch_in(CONTEXT_SWITCH_UNBOUND_FIRST, 0, 0, 0);

    DPRINTF("vmpu_enumerate_boxes [DONE]\n");
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

    /* Check fault register; the following two configurations are allowed.
     *   - Precise data bus fault, no stacking/unstacking errors
     *   - Imprecise data bus fault, no stacking/unstacking errors */
    /* Note: Currently the faulting address argument is not used, since it
     * is saved in r0 for managed bus faults. */
    switch (fault_status) {
        case (SCB_CFSR_MMARVALID_Msk | SCB_CFSR_DACCVIOL_Msk):
            /* Precise data bus fault, no stacking/unstacking errors */
            cnt_max = 0;
            break;

        /* Shift right by a byte because our BFSR (read into fault_status) is
         * already shifted relative to CFSR. The CMSIS masks are CFSR relative,
         * so we need to shift the mask to align with our BFSR. */
        case (SCB_CFSR_IMPRECISERR_Msk >> 8):
            /* Imprecise data bus fault, no stacking/unstacking errors */
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
                    /* DPRINTF("Executed privileged access: 0x%08X written to 0x%08X\n", r1, r0); */
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
                    /* DPRINTF("Executed privileged access: read 0x%08X from 0x%08X\n", r1, r0); */
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
    return vmpu_check_sanity();
}

void vmpu_init_post(void)
{
#if !defined(ARCH_MPU_ARMv8M)
    /* Enable non-base thread mode (NONBASETHRDENA).
     * Exceptions can now return to thread mode regardless their origin
     * (supervisor or thread mode); the opposite is not true. */
    SCB->CCR |= SCB_CCR_NONBASETHRDENA_Msk;
#endif

    /* init memory protection */
    vmpu_arch_init();

    /* load boxes */
    vmpu_enumerate_boxes();
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
    HALT_ERROR(SANITY_CHECK_FAILED, "vmpu: Box namespace missing terminating-null\n");

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

int vmpu_box_id_from_namespace(int * const box_id, const char * const query_namespace)
{
    if (query_namespace == NULL) {
        /* You can't search for anonymous boxes, that would defeat their purpose! */
        return UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS;
    }
    if (box_id == NULL) {
        return UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS;
    }
    const UvisorBoxConfig * * box_cfgtbl;
    box_cfgtbl = (const UvisorBoxConfig * *) __uvisor_config.cfgtbl_ptr_start;

    for (size_t id = 0; id < UVISOR_MAX_BOXES; id++) {
        const char * const current_namespace = box_cfgtbl[id]->box_namespace;
        if (current_namespace == NULL) {
            /* You can't contact anonymous boxes, they contact you! */
            continue;
        }
        /* strlen + 1, since we want to check for \0 as well!
         * strnlen counts the length without terminating NULL, but
         * UVISOR_MAX_BOX_NAMESPACE_LENGTH includes the terminating NULL, therefore -1 */
        size_t max_length = strnlen(current_namespace, UVISOR_MAX_BOX_NAMESPACE_LENGTH - 1) + 1;
        if (!vmpu_priv_unpriv_memcmp((uint32_t) current_namespace,
                                     (uint32_t) query_namespace,
                                     max_length)) {
            /* We found a match! */
            vmpu_unpriv_uint32_write((uint32_t) box_id, id);
            return 0;
        }
    }
    /* No match found. */
    return UVISOR_ERROR_INVALID_BOX_ID;
}

int vmpu_xpriv_memcmp(uint32_t addr1, uint32_t addr2, size_t length, XPrivMemCmpType type)
{
    int32_t data1, data2;
    for (size_t ii = 0; ii < length; ii++) {
        data1 = (type & XPRIV_MEMCMP_ADDR1_PRIVILEGED) ?
                    ((uint8_t *) addr1)[ii] :
                    vmpu_unpriv_uint8_read(addr1 + ii);
        data2 = (type & XPRIV_MEMCMP_ADDR2_PRIVILEGED) ?
                    ((uint8_t *) addr2)[ii] :
                    vmpu_unpriv_uint8_read(addr2 + ii);
        data1 -= data2;
        if (data1) {
            /* <0 if d1 < d2,
             * >0 if d1 > d2  */
            return data1;
        }
    }
    return 0;
}
