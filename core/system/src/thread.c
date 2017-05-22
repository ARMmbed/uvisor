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
#include <uvisor.h>
// #include "api/inc/vmpu_exports.h"
#include "context.h"
#include "halt.h"
#include "ipc.h"
#include "vmpu.h"
#include "rpc.h"

/* Wrap a function call into an atomic section with IRQ state restoration. */
#define atomic_call_wrapper(fn) { \
        uint32_t primask = __get_PRIMASK(); \
        __disable_irq(); \
        fn; \
        if (!(primask & 0x01)) { \
            __enable_irq(); \
        } \
    }

/* By default a maximum of 16 threads are allowed. This can only be overridden
 * by the porting engineer for the current platform. */
#ifndef UVISOR_EXPORT_TABLE_THREADS_MAX_COUNT
#define UVISOR_EXPORT_TABLE_THREADS_MAX_COUNT ((uint32_t) 16)
#endif

/* Per thread we store the pointer to the allocator and the process id that
 * this thread belongs to. */
typedef struct {
    void * allocator;
    int process_id;
} UvisorThreadContext;

static UvisorThreadContext thread[UVISOR_EXPORT_TABLE_THREADS_MAX_COUNT];


static int thread_ctx_valid(UvisorThreadContext * context)
{
    /* Check if context pointer points into the array. */
    if ((uint32_t) context < (uint32_t) &thread ||
        ((uint32_t) &thread + sizeof(thread)) <= (uint32_t) context) {
        return 0;
    }
    /* Check if the context is aligned exactly to a context. */
    return (((uint32_t) context - (uint32_t) thread) % sizeof(UvisorThreadContext)) == 0;
}

void * thread_create(int id, void * c)
{
    (void) id;
    UvisorThreadContext * context = c;
    const UvisorBoxIndex * const index =
            (UvisorBoxIndex * const) *(__uvisor_config.uvisor_box_context);
    /* Search for a free slot in the tasks meta data. */
    uint32_t ii = 0;
    for (; ii < UVISOR_EXPORT_TABLE_THREADS_MAX_COUNT; ii++) {
        if (thread[ii].allocator == NULL) {
            break;
        }
    }
    if (ii < UVISOR_EXPORT_TABLE_THREADS_MAX_COUNT) {
        /* Remember the process id for this thread. */
        thread[ii].process_id = g_active_box;
        /* Fall back to the process heap if ctx is NULL. */
        thread[ii].allocator = context ? context : (void *) index->bss.address_of.heap;
        return &thread[ii];
    }
    return context;
}

void thread_destroy(void * c)
{
    UvisorThreadContext * context = c;
    if (context == NULL) {
        return;
    }

    /* Only if TID is valid and destruction status is zero. */
    if (thread_ctx_valid(context) && context->allocator && (context->process_id == g_active_box)) {
        /* Release this slot. */
        context->allocator = NULL;
    } else {
        /* This is a debug only assertion, not present in release builds, to
         * prevent a malicious box from taking down the entire system by
         * fiddling with one of its thread contexts or destroying another box's
         * thread. */
        assert(false);
    }
}

static void thread_switch_nonatomic(void * c)
{
    UvisorThreadContext * context = c;
    UvisorBoxIndex * index;

    /* Drain any outgoing RPC queues */
    drain_message_queue();
    drain_result_queue();

    /* Drain the IPC queue. */
    ipc_drain_queue();

    if (context == NULL) {
        return;
    }

    /* Only if TID is valid and the slot is used */
    if (!thread_ctx_valid(context) || context->allocator == NULL) {
        /* This is a debug only assertion, not present in release builds, to
         * prevent a malicious box from taking down the entire system by
         * fiddling with one of its thread contexts. */
        assert(false);
        return;
    }
    /* If the thread is inside another process, switch into it. */
    if (context->process_id != g_active_box) {
        context_switch_in(CONTEXT_SWITCH_UNBOUND_THREAD, context->process_id, 0, 0);
    }
    /* Copy the thread allocator into the (new) box index. */
    /* Note: The value in index is updated by context_switch_in, or is already
     *       the correct one if no switch needs to occur. */
    index = (UvisorBoxIndex *) *(__uvisor_config.uvisor_box_context);
    if (context->allocator) {
        /* If the active_heap is NULL, then the process heap needs to be
         * initialized yet. The initializer sets the active heap itself. */
        if (index->active_heap) {
            index->active_heap = context->allocator;
        }
    }
}

/* uVisor expects all calls to its API to be executed in the highest priority
 * interrupt (SVC) since they are not re-entrant.
 * If this is not the case, we need to make sure that no other interrupt can
 * preempt these calls by wrapping them in an atomic section.
 */
void thread_switch(void * c)
{
    atomic_call_wrapper(
        thread_switch_nonatomic(c)
    );
}
