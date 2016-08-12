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
#include "api/inc/export_table_exports.h"
#include "api/inc/svc_exports.h"
#include "api/inc/vmpu_exports.h"
#include "context.h"
#include "halt.h"

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

static void * thread_create(int id, void * c)
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
        thread[ii].allocator = context ? context : index->box_heap;
        return &thread[ii];
    }
    return context;
}

static void thread_destroy(void * c)
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

static void thread_switch(void * c)
{
    UvisorThreadContext * context = c;
    UvisorBoxIndex * index;
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

static void boxes_init(void)
{
    /* Tell uVisor to call the uVisor lib box_init function for each box with
     * each box's uVisor lib config. */

    /* This must be called from unprivileged mode in order for the recursive
     * gateway chaining to work properly. */
    UVISOR_SVC(UVISOR_SVC_ID_BOX_INIT_FIRST, "");
}

/* This table must be located at the end of the uVisor binary so that this
 * table can be exported correctly. Placing this table into the .export_table
 * section locates this table at the end of the uVisor binary. */
__attribute__((section(".export_table")))
const TUvisorExportTable __uvisor_export_table = {
    .magic = UVISOR_EXPORT_MAGIC,
    .version = UVISOR_EXPORT_VERSION,
    .os_event_observer = {
        .version = 0,
        .pre_start = boxes_init,
        .thread_create = thread_create,
        .thread_destroy = thread_destroy,
        .thread_switch = thread_switch,
    },
    .pool = {
        .init = uvisor_pool_init,
        .queue_init = uvisor_pool_queue_init,
        .allocate = uvisor_pool_allocate,
        .queue_enqueue = uvisor_pool_queue_enqueue,
        .free = uvisor_pool_free,
        .queue_dequeue = uvisor_pool_queue_dequeue,
        .queue_dequeue_first = uvisor_pool_queue_dequeue_first,
        .queue_find_first = uvisor_pool_queue_find_first,
    },
    .size = sizeof(TUvisorExportTable)
};
