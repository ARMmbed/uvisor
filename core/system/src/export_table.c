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
#include "semaphore.h"
#include "api/inc/export_table_exports.h"
#include "api/inc/rpc_gateway_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/rpc_exports.h"
#include "api/inc/svc_exports.h"
#include "api/inc/vmpu_exports.h"
#include "api/inc/register_gateway.h"
#include "context.h"
#include "halt.h"
#include "vmpu.h"

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

/* Wake up all the potential handlers for this RPC target. Return number of
 * handlers posted to. */
static int wake_up_handlers_for_target(const TFN_Ptr function, int box_id)
{
    int num_posted = 0;

    UvisorBoxIndex * index = (UvisorBoxIndex *) g_context_current_states[box_id].bss;
    uvisor_pool_queue_t * fn_group_queue = &index->rpc_fn_group_queue->queue;
    uvisor_rpc_fn_group_t * fn_group_array = index->rpc_fn_group_queue->fn_groups;

    /* Wake up all known waiters for this function. Search for the function in
     * all known function groups. We have to search through all function groups
     * (not just those currently waiting for messages) because we want the RTOS
     * to be able to pick the highest priority waiter to schedule to run. Some
     * waiters will wake up and find they have nothing to do if a higher
     * priority waiter already took care of handling the incoming RPC. */
    uvisor_pool_slot_t slot;
    slot = fn_group_queue->head;
    while (slot < fn_group_queue->pool->num) {
        /* Look for the function in this function group. */
        uvisor_rpc_fn_group_t * fn_group = &fn_group_array[slot];

        TFN_Ptr const * fn_ptr_array = fn_group->fn_ptr_array;
        size_t i;

        for (i = 0; i < fn_group->fn_count; i++) {
            /* If function is found: */
            if (fn_ptr_array[i] == function) {
                /* Wake up the waiter. */
                semaphore_post(&fn_group->semaphore);
                ++num_posted;
            }
        }

        slot = fn_group_queue->pool->management_array[slot].queued.next;
    }

    return num_posted;
}

static int callee_box_id(const TRPCGateway * gateway)
{
    int box_id;

    /* Make sure the gateway box_ptr is aligned to 4 bytes. */
    if (gateway->box_ptr & 0x3) {
        return -1;
    }

    box_id = (uint32_t *)gateway->box_ptr - __uvisor_config.cfgtbl_ptr_start;

    /* Make sure the box_id we compute is within the range of valid boxes. Box
     * 0 cannot be a callee, so valid range is between 1 (inclusive) and the
     * box count (exclusive); [1, g_vmpu_box_count). */
    if (box_id < 1 || box_id >= g_vmpu_box_count) {
        return -1;
    }

    return box_id;
}

static int put_it_back(uvisor_pool_queue_t * queue, uvisor_pool_slot_t slot)
{
    int status;
    status = uvisor_pool_queue_try_enqueue(queue, slot);
    if (status) {
        /* We could dequeue an RPC message, but couldn't put it back. */
        /* It is bad to take down the entire system. It is also bad
         * to lose messages due to not being able to put them back in
         * the queue. However, if we could dequeue the slot
         * we should have no trouble enqueuing the slot here. */
        assert(false);
    }

    /* Note that we don't have to modify the message in the queue, since it'll
     * still be valid. Nobody else will have run at the same time that could
     * have messed it up. */

     return status;
}

/* Return true iff gateway is valid. */
static int is_valid_rpc_gateway(const TRPCGateway * const gateway)
{
    /* Gateway needs to be entirely in flash. */
    uint32_t gateway_start = (uint32_t) gateway;
    uint32_t gateway_end = gateway_start + sizeof(*gateway);
    if (!(vmpu_public_flash_addr(gateway_start) && vmpu_public_flash_addr(gateway_end))) {
        return 0;
    }

    /* Gateway needs to have good magic. */
    if (!(gateway->magic == UVISOR_RPC_GATEWAY_MAGIC_ASYNC || gateway->magic == UVISOR_RPC_GATEWAY_MAGIC_SYNC)) {
        return 0;
    }

    /* Gateway needs good ldr_pc instruction. */
    if (gateway->ldr_pc != LDR_PC_PC_IMM_OPCODE(__UVISOR_OFFSETOF(TRPCGateway, ldr_pc),
                                                __UVISOR_OFFSETOF(TRPCGateway, caller))) {
        return 0;
    }

    /* The callee box ID associated with the gateway must be valid. */
    int box_id = callee_box_id(gateway);
    if (box_id < 0) {
        return 0;
    }

    /* Gateway needs to point to functions in flash (caller and target) */
    if (!(vmpu_public_flash_addr(gateway->caller) && vmpu_public_flash_addr(gateway->target))) {
        return 0;
    }

    /* All checks passed. */
    return 1;
}

/* Return true if and only if the queue is entirely within the box specified by
 * the provided box_id. */
static int is_valid_queue(uvisor_pool_queue_t * queue, int box_id)
{
    uint32_t bss_start = g_context_current_states[box_id].bss;
    uint32_t bss_end = bss_start + g_context_current_states[box_id].bss_size;

    uint32_t queue_start = (uint32_t) queue;
    uint32_t queue_end = queue_start + sizeof(*queue);
    int queue_belongs_to_box = queue_start >= bss_start && queue_end <= bss_end;
    if (!queue_belongs_to_box) {
        return 0;
    }

    if (queue->magic != UVISOR_POOL_QUEUE_MAGIC) {
        /* Queue lacked good magic. */
        return 0;
    }

    uint32_t pool_start = (uint32_t) queue->pool;
    uint32_t pool_end = pool_start + sizeof(*queue->pool);
    int pool_belongs_to_box = (pool_start >= bss_start) && (pool_end <= bss_end);
    if (!pool_belongs_to_box) {
        return 0;
    }

    if (queue->pool->magic != UVISOR_POOL_MAGIC) {
        /* Pool lacked good magic. */
        return 0;
    }

    uint32_t man_array_start = (uint32_t) queue->pool->management_array;
    uint32_t man_array_end = man_array_start + sizeof(*queue->pool->management_array) * queue->pool->num;
    int man_array_is_valid = (man_array_start >= bss_start) && (man_array_end <= bss_end);
    if (!man_array_is_valid) {
        return 0;
    }

    uint32_t array_start = (uint32_t) queue->pool->array;
    uint32_t array_end = array_start + queue->pool->stride * queue->pool->num;
    int array_is_valid = (array_start >= bss_start) && (array_end <= bss_end);
    if (!array_is_valid) {
        return 0;
    }

    /* We made it through all the checks. */
    return 1;
}

static void drain_message_queue(void)
{
    UvisorBoxIndex * caller_index = (UvisorBoxIndex *) *__uvisor_config.uvisor_box_context;
    uvisor_pool_queue_t * caller_queue = &caller_index->rpc_outgoing_message_queue->queue;
    uvisor_rpc_message_t * caller_array = (uvisor_rpc_message_t *) caller_queue->pool->array;
    int caller_box = g_active_box;
    int first_slot = -1;

    /* Verify that the caller queue is entirely in caller box BSS. We check the
     * entire queue instead of just the message we are interested in, because
     * we want to validate the queue before we attempt any operations on it,
     * like dequeing. */
    if (!is_valid_queue(caller_queue, caller_box))
    {
        /* The caller's outgoing queue is not valid. This shouldn't happen in a
         * non-malicious system. */
        assert(false);
        return;
    }

    /* For each message in the queue: */
    do {
        uvisor_pool_slot_t caller_slot;

        /* NOTE: We only dequeue the message from the queue. We don't free
         * the message from the pool. The caller will free the message from the
         * pool after finish waiting for the RPC to finish. */
        caller_slot = uvisor_pool_queue_try_dequeue_first(caller_queue);
        if (caller_slot >= caller_queue->pool->num) {
            /* The queue is empty or busy. */
            break;
        }

        /* If we have seen this slot before, stop processing the queue. */
        if (first_slot == -1) {
            first_slot = caller_slot;
        } else if (caller_slot == first_slot) {
            put_it_back(caller_queue, caller_slot);

            /* Stop looping, because the system needs to continue running so
             * the callee messages can get processed to free up more room.
             * */
            break;
        }

        /* We would like to finish processing all messages in the queue, even
         * if one can't be delivered now. We currently just stop when we can't
         * deliver one message and never attempt the rest. */

        uvisor_rpc_message_t * caller_msg = &caller_array[caller_slot];

        /* Validate the gateway */
        const TRPCGateway * const gateway = caller_msg->gateway;
        if (!is_valid_rpc_gateway(gateway)) {
            /* The RPC gateway is not valid. Don't put the message back onto
             * the queue. Move on to next items. On a non-malicious system, the
             * gateway should always be valid here. */
            assert(false);
            continue;
        }

        /* Look up the callee box. */
        const int callee_box = callee_box_id(gateway);
        if (callee_box < 0) {
            /* This shouldn't happen, because the gateway was already verified.
             * */
            assert(false);
            continue;
        }

        UvisorBoxIndex * callee_index = (UvisorBoxIndex *) g_context_current_states[callee_box].bss;
        uvisor_pool_queue_t * callee_queue = &callee_index->rpc_incoming_message_queue->todo_queue;
        uvisor_rpc_message_t * callee_array = (uvisor_rpc_message_t *) callee_queue->pool->array;

        /* Verify that the callee queue is entirely in callee box BSS. We check the
         * entire queue instead of just the message we are interested in, because
         * we want to validate the queue before we attempt any operations on it,
         * like allocating. */
        if (!is_valid_queue(callee_queue, callee_box))
        {
            /* The callee's todo queue is not valid. This shouldn't happen in a
             * non-malicious system. Don't put the caller's message back into
             * the queue; this is the same behavior (from the caller's
             * perspective) as a malicious box never completing RPCs. */
            assert(false);
            continue;
        }

        /* Place the message into the callee box queue. */
        uvisor_pool_slot_t callee_slot = uvisor_pool_queue_try_allocate(callee_queue);

        /* If the queue is not busy and there is space in the callee queue: */
        if (callee_slot < callee_queue->pool->num)
        {
            int status;
            uvisor_rpc_message_t * callee_msg = &callee_array[callee_slot];

            /* Deliver the message. */
            callee_msg->p0 = caller_msg->p0;
            callee_msg->p1 = caller_msg->p1;
            callee_msg->p2 = caller_msg->p2;
            callee_msg->p3 = caller_msg->p3;
            callee_msg->gateway = caller_msg->gateway;
            /* Set the ID of the calling box in the message. */
            callee_msg->other_box_id = caller_box;
            callee_msg->match_cookie = caller_msg->match_cookie;
            callee_msg->state = UVISOR_RPC_MESSAGE_STATE_SENT;

            caller_msg->other_box_id = callee_box;
            caller_msg->state = UVISOR_RPC_MESSAGE_STATE_SENT;

            /* Enqueue the message */
            status = uvisor_pool_queue_try_enqueue(callee_queue, callee_slot);
            /* We should always be able to enqueue, since we were able to
             * allocate the slot. Nobody else should have been able to run and
             * take the spin lock. */
            if (status) {
                /* We were able to get the callee RPC slot allocated, but
                 * couldn't enqueue the message. It is bad to take down the
                 * entire system. It is also bad to keep the allocated slot
                 * around. However, if we couldn't enqueue the slot, we'll have
                 * a hard time freeing it, since that requires the same lock.
                 * */
                assert(false);

                /* Put the message back into the queue, as we may be able to
                 * enqueue the message when we try again later. This is likely
                 * to fail as well, if we couldn't enqueue the message. However,
                 * if we can't put it back now, there is nothing we can do and
                 * the message must be lost. */
                put_it_back(caller_queue, caller_slot);
                continue;
            }

            /* Poke anybody waiting on calls to this target function. If nobody
             * is waiting, the item will remain in the incoming queue. The
             * first time a rpc_fncall_waitfor is called for a function group,
             * rpc_fncall_waitfor will check to see if there are any messages
             * it can handle from before the function group existed. */
            wake_up_handlers_for_target((TFN_Ptr)gateway->target, callee_box);
        }

        /* If there was no room in the callee queue: */
        if (callee_slot >= callee_queue->pool->num)
        {
            /* Put the message back into the caller queue. This applies
             * backpressure on the caller when the callee is too busy. Note
             * that no data needs to be copied; only the caller queue's
             * management array is modified. */
            put_it_back(caller_queue, caller_slot);
        }
    } while (1);
}

static void drain_result_queue(void)
{
    UvisorBoxIndex * callee_index = (UvisorBoxIndex *) *__uvisor_config.uvisor_box_context;
    uvisor_pool_queue_t * callee_queue = &callee_index->rpc_incoming_message_queue->done_queue;
    uvisor_rpc_message_t * callee_array = (uvisor_rpc_message_t *) callee_queue->pool->array;

    int callee_box = g_active_box;

    /* Verify that the callee queue is entirely in caller box BSS. We check the
     * entire queue instead of just the message we are interested in, because
     * we want to validate the queue before we attempt any operations on it,
     * like dequeing. */
    if (!is_valid_queue(callee_queue, callee_box))
    {
        /* The callee's done queue is not valid. This shouldn't happen in a
         * non-malicious system. */
        assert(false);
        return;
    }

    /* For each message in the queue: */
    do {
        uvisor_pool_slot_t callee_slot;

        /* Dequeue the first result message from the queue. */
        callee_slot = uvisor_pool_queue_try_dequeue_first(callee_queue);
        if (callee_slot >= callee_queue->pool->num) {
            /* The queue is empty or busy. */
            break;
        }

        uvisor_rpc_message_t * callee_msg = &callee_array[callee_slot];

        /* Look up the origin message. This should have been remembered
         * by uVisor when it did the initial delivery. */
        uvisor_pool_slot_t caller_slot = uvisor_result_slot(callee_msg->match_cookie);


        /* Based on the origin message, look up the box to return the result to
         * (caller box). */
        const int caller_box = callee_msg->other_box_id;

        UvisorBoxIndex * caller_index = (UvisorBoxIndex *) g_context_current_states[caller_box].bss;
        uvisor_pool_queue_t * caller_queue = &caller_index->rpc_outgoing_message_queue->queue;
        uvisor_rpc_message_t * caller_array = (uvisor_rpc_message_t *) caller_queue->pool->array;

        /* Verify that the caller queue is entirely in caller box BSS. We check the
         * entire queue instead of just the message we are interested in, because
         * we want to validate the queue before we attempt any operations on it. */
        if (!is_valid_queue(caller_queue, caller_box))
        {
            /* The caller's outgoing queue is not valid. The caller queue is
             * messed up. This shouldn't happen in a non-malicious system.
             * Discard the result message (not retrying later), because the
             * caller is malicious. */
            assert(false);
            continue;
        }

        uvisor_rpc_message_t * caller_msg = &caller_array[caller_slot];

        /* Verify that the caller box is waiting for the callee box to complete
         * the RPC in this slot. */

        /* Other box ID must be same. */
        if (caller_msg->other_box_id != callee_box) {
            /* The caller isn't waiting for this box to complete it. This
             * shouldn't happen in a non-malicious system. */
            assert(false);
            continue;
        }

        /* The caller must be waiting for a box to complete this slot. */
        if (caller_msg->state != UVISOR_RPC_MESSAGE_STATE_SENT)
        {
            /* The caller isn't waiting for any box to complete it. This
             * shouldn't happen in a non-malicious system. */
            assert(false);
            continue;
        }

        /* The match_cookie must be same. */
        if (caller_msg->match_cookie != callee_msg->match_cookie) {
            /* The match cookies didn't match. This shouldn't happen in a
             * non-malicious system. */
            assert(false);
            continue;
        }

        /* Copy the result to the message in the caller box outgoing message
         * queue. */
        caller_msg->result = callee_msg->result;
        callee_msg->state = UVISOR_RPC_MESSAGE_STATE_IDLE;
        caller_msg->state = UVISOR_RPC_MESSAGE_STATE_DONE;

        /* Now that we've copied the result, we can free the message from the
         * callee queue. The callee (the one sending result messages) doesn't
         * care about the message after they post it to their outgoing result
         * queue. */
        callee_slot = uvisor_pool_queue_try_free(callee_queue, callee_slot);
        if (callee_slot >= callee_queue->pool->num) {
            /* The queue is empty or busy. This should never happen. We were
             * able to dequeue a result message, but weren't able to free the
             * result message. It is bad to take down the entire system. It is
             * also bad to never free slots in the outgoing result queue.
             * However, if we could dequeue the slot we should have no trouble
             * freeing the slot here. */
            assert(false);
            break;
        }

        /* Post to the result semaphore, ignoring errors. */
        int status;
        status = semaphore_post(&caller_msg->semaphore);
        if (status) {
            /* We couldn't post to the result semaphore. We shouldn't really
             * bring down the entire system if one box messes up its own
             * semaphore. In a non-malicious system, this should never happen.
             * */
            assert(false);
        }
    } while (1);
}

static void drain_outgoing_rpc_queues(void)
{
    drain_message_queue();
    drain_result_queue();
}

static void thread_switch(void * c)
{
    UvisorThreadContext * context = c;
    UvisorBoxIndex * index;

    /* Drain any outgoing RPC queues */
    drain_outgoing_rpc_queues();

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
