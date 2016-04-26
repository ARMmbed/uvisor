/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#include "api/inc/uvisor-lib.h"
#include "core/uvisor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Number of implemented priority bits */
uint8_t g_nvic_prio_bits;

/* Symbols exported by the mbed linker script */
UVISOR_EXTERN uint32_t __uvisor_cfgtbl_ptr_start;
UVISOR_EXTERN uint32_t __uvisor_cfgtbl_ptr_end;
UVISOR_EXTERN uint32_t __uvisor_bss_boxes_start;

/* The pointer to the uVisor context is declared by each box separately. Each
 * declaration will have its own type. */
void *uvisor_ctx;

/* Flag to check that contexts have been initialized */
static bool g_initialized = false;

/* Array with all box context pointers */
static void *g_uvisor_ctx_array[UVISOR_MAX_BOXES] = {0};

/* Call stack
 * We must keep the full call stack as otherwise it's not possible to restore a
 * box context after a nested call. */
static uint8_t g_call_stack[UVISOR_SVC_CONTEXT_MAX_DEPTH];
static int g_call_sp;

/* Memory position pointer
 * It is used to allocate chunks of memories from the allocated pool (uVisor
 * boxes' .bss). */
static uint32_t g_memory_position;

/* Forward declaration for the internal IRQ handlers */
static void __not_implemented(void);
static void uvisor_disabled_default_vector(void);

/* Local vector table
 * Whatever the host OS vector table is, we will relocate to this one. */
static const __attribute__((aligned(512), section(".data"))) uint32_t g_irq_table[NVIC_OFFSET + NVIC_VECTORS] = {
    [0 ... NVIC_OFFSET - 1] = (uint32_t) __not_implemented,
    [NVIC_OFFSET ... NVIC_OFFSET + NVIC_VECTORS - 1] = (uint32_t) &uvisor_disabled_default_vector,
};

/* Dynamic IRQ table
 * It holds the box ID and the vector for all registered IRQs, so that a context
 * switch can be triggered before serving an interrupt. */
/* Note: The ownership information only applies to NVIC IRQs, not system ones. */
typedef struct {
    uint8_t box_id;
    uint32_t vector;
} TUserIrq;
TUserIrq g_uvisor_disabled_vectors[NVIC_VECTORS];

static void __not_implemented(void)
{
    while(1);
}

static void uvisor_disabled_init_context(void)
{
    uint8_t box_id;
    const UvisorBoxConfig **box_cfgtbl;
    size_t context_size;

    if (g_initialized) {
        return;
    }

    /* Iterate over all box configuration tables. */
    box_id = 0;
    g_memory_position = (uint32_t) &__uvisor_bss_boxes_start;
    for(box_cfgtbl = (const UvisorBoxConfig**) &__uvisor_cfgtbl_ptr_start;
        box_cfgtbl < (const UvisorBoxConfig**) &__uvisor_cfgtbl_ptr_end;
        box_cfgtbl++) {
        /* Read the context size from the box configuration table. */
        context_size = (size_t) (*box_cfgtbl)->context_size;

        /* Initialize box context. */
        /* Note: Also box 0 has technically a context, although we force it to
         *       be zero. */
        if (!context_size) {
            g_uvisor_ctx_array[box_id] = NULL;
        } else if (!box_id) {
            uvisor_error(USER_NOT_ALLOWED);
        } else {
            /* The box context is alloated from the chunk of memory reserved to
             * uVisor boxes' stacks and contexts. */
            /* FIXME Since we do not currently track separate stacks when uVisor
             * is disabled, this involves a good wealth of memory waste. */
            g_uvisor_ctx_array[box_id] = (void *) g_memory_position;
            memset((void *) g_memory_position, 0, UVISOR_REGION_ROUND_UP(context_size));
            g_memory_position += UVISOR_REGION_ROUND_UP(context_size);
        }
        box_id++;
    }

    /* Do not run this again. */
    g_initialized = true;
}

void uvisor_disabled_switch_in(const uint32_t *dst_box_cfgtbl_ptr)
{
    uint8_t dst_box_id;

    /* Read the destination box ID. */
    dst_box_id = (uint8_t) (dst_box_cfgtbl_ptr - &__uvisor_cfgtbl_ptr_start);

    /* Allocate the box contexts if they do not exist yet. */
    if (!g_initialized) {
        uvisor_disabled_init_context();
    }

    uvisor_ctx = g_uvisor_ctx_array[dst_box_id];

    /* Push state. */
    if (g_call_sp >= UVISOR_SVC_CONTEXT_MAX_DEPTH - 1) {
        uvisor_error(USER_NOT_ALLOWED);
    }
    g_call_stack[++g_call_sp] = dst_box_id;
}

void uvisor_disabled_switch_out(void)
{
    uint8_t src_box_id;

    /* Pop state. */
    if (g_call_sp <= 0) {
        uvisor_error(USER_NOT_ALLOWED);
    }
    src_box_id = g_call_stack[--g_call_sp];

    /* Restore the source context. */
    uvisor_ctx = g_uvisor_ctx_array[src_box_id];
}

static void uvisor_disabled_default_vector(void)
{
    uint32_t irqn, vector;
    const uint32_t *dst_box_cfgtbl_ptr;
    uint8_t dst_box_id;
    int ipsr;

    /* Get current IRQ number, from the IPSR.
     * We only allow user IRQs to be registered (NVIC). This is consistent with
     * the corresponding API when uVisor is enabled. */
    irqn = 0;
    ipsr = ((int) (__get_IPSR() & 0x1FF)) - NVIC_OFFSET;
    if (ipsr < 0 || ipsr >= NVIC_VECTORS) {
        uvisor_error(USER_NOT_ALLOWED);
    } else {
        irqn = (uint32_t) ipsr;
    }

    /* Calculate the destination box configuration pointer. */
    dst_box_id = g_uvisor_disabled_vectors[irqn].box_id;
    dst_box_cfgtbl_ptr = &__uvisor_cfgtbl_ptr_start + (uint32_t) dst_box_id;

    /* Get the IRQ handler. */
    vector = g_uvisor_disabled_vectors[irqn].vector;
    if (!vector) {
        uvisor_error(USER_NOT_ALLOWED);
    }

    /* Switch contexts before and after executing the IRQ handler. */
    uvisor_disabled_switch_in(dst_box_cfgtbl_ptr);
    ((void (*)(void)) vector)();
    uvisor_disabled_switch_out();
}

UVISOR_WEAK void uvisor_disabled_set_vector(uint32_t irqn, uint32_t vector)
{
    uint8_t prio_bits, box_id;
    uint8_t volatile *prio;

    /* Check IRQn.
     * We only allow user IRQs to be registered (NVIC). This is consistent with
     * the corresponding API when uVisor is enabled. */
    if (irqn >= NVIC_VECTORS) {
        uvisor_error(USER_NOT_ALLOWED);
    }

    /* Get current box ID.
     * We use the call stack pointer to assess the currently active box ID. */
    box_id = g_call_stack[g_call_sp];

    /* Setup the vector table relocation (only done once).
     * No user vector is copied, consistently with the vIRQ APIs. Instead, only
     * user vectors explicitly set using this API are registered in the table. */
    if (SCB->VTOR != (uint32_t) g_irq_table) {
        SCB->VTOR = (uint32_t) g_irq_table;

        /* Detect the number of implemented priority bits.
         * The architecture specifies that unused/not implemented bits in the
         * NVIC IP registers read back as 0. */
        __disable_irq();
        prio = (uint8_t volatile *) &(NVIC->IP[0]);
        prio_bits = *prio;
        *prio = 0xFFU;
        g_nvic_prio_bits = (uint8_t) __builtin_popcount(*prio);
        *prio = prio_bits;
        __enable_irq();
    }

    /* Register IRQ.
     * If vector is 0 it corresponds to a de-registration. */
    g_uvisor_disabled_vectors[irqn].box_id = vector ? box_id : 0;
    g_uvisor_disabled_vectors[irqn].vector = vector;
}

UVISOR_WEAK uint32_t uvisor_disabled_get_vector(uint32_t irqn)
{
    /* Check IRQn.
     * We only allow user IRQs to be registered (NVIC). This is consistent with
     * the corresponding API when uVisor is enabled. */
    if (irqn >= NVIC_VECTORS) {
        uvisor_error(USER_NOT_ALLOWED);
    }

    return g_uvisor_disabled_vectors[irqn].vector;
}
