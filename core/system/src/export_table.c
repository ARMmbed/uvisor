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
#include "api/inc/vmpu_exports.h"
#include "context.h"
#include "halt.h"

typedef struct {
    void *allocator;
    uint8_t process_id;
    int thread_id;
} UvisorThreadContext;

#define UVISOR_MAX_THREADS 20
static UvisorThreadContext thread[UVISOR_MAX_THREADS];

static int thread_ctx_valid(UvisorThreadContext *context)
{
    /* check if context pointer points into the array */
    if ((void *)context < (void *)&thread ||
        ((void *)&thread + sizeof(thread)) <= (void *)context) {
        return 0;
    }
    /* check if the context is aligned exactly to a context */
    return (((void *)context - (void *)thread) % sizeof(UvisorThreadContext)) == 0;
}

static void *thread_create(int id, void *context)
{
    const UvisorBoxIndex *const index =
            (UvisorBoxIndex *const) *(__uvisor_config.uvisor_box_context);
    /* search for a free slot in the tasks meta data */
    int ii = 0;
    for (; ii < UVISOR_MAX_THREADS; ii++) {
        if (thread[ii].allocator == NULL)
            break;
    }
    if (ii < UVISOR_MAX_THREADS) {
        thread[ii].thread_id = id;
        /* remember the process id as well */
        thread[ii].process_id = g_active_box;
        /* fall back to the process heap if ctx is NULL */
        thread[ii].allocator = context ? context : index->box_heap;
        return &thread[ii];
    }
    return context;
}

static void thread_destroy(UvisorThreadContext *context)
{
    if (context == NULL) return;

    /* only if TID is valid and destruction status is zero */
    if (thread_ctx_valid(context) &&
        (context->process_id == g_active_box)) {
        /* release this slot */
        context->allocator = NULL;
    } else {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "thread context (%08x) is invalid!\n",
            context);
    }
}

static void thread_switch(UvisorThreadContext *context)
{
    UvisorBoxIndex *index;
    if (context == NULL) return;


    /* Only if TID is valid and the slot is used */
    if (!thread_ctx_valid(context)) {
        HALT_ERROR(SANITY_CHECK_FAILED,
            "thread context (%08x) is invalid!\n",
            context);
        return;
    }
    if (context->process_id != g_active_box) {
        context_switch_in(CONTEXT_SWITCH_UNBOUND_THREAD, context->process_id, 0, 0);
    }
    index = (UvisorBoxIndex *) *(__uvisor_config.uvisor_box_context);
    if (context->allocator) {
        /* If the active_heap is NULL, then the process heap needs to be
         * initialized yet. The initializer sets the active heap itself. */
        if (index->active_heap) {
            index->active_heap = context->allocator;
        }
    }
}

/* This table must be located at the end of the uVisor binary so that this
 * table can be exported correctly. Placing this table into the .export_table
 * section locates this table at the end of the uVisor binary. */
__attribute__((section(".export_table")))
static const TUvisorExportTable __uvisor_export_table = {
    .magic = UVISOR_EXPORT_MAGIC,
    .version = UVISOR_EXPORT_VERSION,
    .thread_observer = {
        .version = 0,
        .thread_create = thread_create,
        .thread_destroy = (void (*)(void *)) thread_destroy,
        .thread_switch = (void (*)(void *)) thread_switch,
    },
    .size = sizeof(TUvisorExportTable)
};
