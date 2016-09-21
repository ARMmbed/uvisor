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
#include "api/inc/rpc.h"
#include "api/inc/rpc_exports.h"
#include "api/inc/rpc_gateway.h"
#include "api/inc/vmpu_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/error.h"
#include "api/inc/uvisor_semaphore.h"
#include <string.h>

extern UvisorBoxIndex * __uvisor_ps;


static uvisor_pool_queue_t * outgoing_message_queue(void)
{
    return &__uvisor_ps->rpc_outgoing_message_queue->queue;
}

static uvisor_rpc_message_t * outgoing_message_array(void)
{
    return __uvisor_ps->rpc_outgoing_message_queue->messages;
}

static uint32_t * result_counter(void)
{
    return &__uvisor_ps->rpc_result_counter;
}

static uvisor_pool_queue_t * incoming_message_todo_queue(void)
{
    return &__uvisor_ps->rpc_incoming_message_queue->todo_queue;
}

static uvisor_pool_queue_t * incoming_message_done_queue(void)
{
    return &__uvisor_ps->rpc_incoming_message_queue->done_queue;
}

static uvisor_rpc_message_t * incoming_message_array(void)
{
    return __uvisor_ps->rpc_incoming_message_queue->messages;
}

static uvisor_pool_queue_t * fn_group_queue(void)
{
    return &__uvisor_ps->rpc_fn_group_queue->queue;
}

static uvisor_rpc_fn_group_t * fn_group_array(void)
{
    return __uvisor_ps->rpc_fn_group_queue->fn_groups;
}

/* Place a message into the outgoing queue. `timeout_ms` is how long to wait
 * for a slot in the outgoing queue before giving up. `msg_slot` is set to the
 * slot of the message that was allocated. Returns non-zero on failure. */
static int send_outgoing_rpc(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, const TRPCGateway * gateway, uint32_t timeout_ms,
                             uvisor_rpc_result_t * cookie)
{
    uint32_t counter;
    uvisor_rpc_message_t * msg;
    uvisor_pool_slot_t slot;

    /* Claim a slot in the outgoing RPC queue. */
    slot = uvisor_pool_queue_allocate(outgoing_message_queue(), timeout_ms);
    if (slot >= outgoing_message_queue()->pool->num) {
        /* No slots available in incoming queue. We asked for a free slot but
         * didn't get one. */
        return -1;
    }

    /* Atomically increment the counter. */
    counter = __sync_add_and_fetch(result_counter(), UVISOR_RESULT_COUNTER_INCREMENT);

    /* Populate the message */
    msg = &outgoing_message_array()[slot];
    msg->p0 = p0;
    msg->p1 = p1;
    msg->p2 = p2;
    msg->p3 = p3;
    msg->gateway = gateway;
    msg->wait_cookie = uvisor_result_build(counter, slot);
    msg->match_cookie = msg->wait_cookie;
    msg->state = UVISOR_RPC_MESSAGE_STATE_READY_TO_SEND;

    /* Put the slot into the queue. */
    uvisor_pool_queue_enqueue(outgoing_message_queue(), slot);

    /* Notify the caller of this function of the slot that was allocated for
     * sending this RPC message. */
    *cookie = msg->match_cookie;

    return 0;
}

/* Wait up to `timeout_ms` for the RPC in `msg_slot` to complete. Return 0 if
 * the RPC completed. */
static int wait_for_rpc_result(uvisor_pool_slot_t msg_slot, uint32_t timeout_ms)
{
    return __uvisor_semaphore_pend(&outgoing_message_array()[msg_slot].semaphore, timeout_ms);
}

static void free_outgoing_msg(uvisor_pool_slot_t msg_slot)
{
    /* We are done with the outgoing RPC message now. uVisor dequeued the RPC
     * message on our behalf when uVisor sent the message to the destination,
     * so we can just free without dequeueing first. */
    if (outgoing_message_queue()->pool->management_array[msg_slot].dequeued.state != UVISOR_POOL_SLOT_IS_DEQUEUED) {
        uvisor_error(USER_NOT_ALLOWED);
    }
    uvisor_pool_queue_free(outgoing_message_queue(), msg_slot);
}

uint32_t rpc_fncall_sync(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, const TRPCGateway * gateway)
{
    int status;
    uint32_t result_value;
    uvisor_rpc_result_t cookie;
    uvisor_pool_slot_t msg_slot;

    /* The synchronous RPC calling function has no way to fail, so it must
     * infinitely retry operations until the RPC succeeds. */

    /* Loop until sending the RPC message succeeds. */
    do {
        /* Because this is the sync function, we use wait forever to wait for an
         * available message slot. */
        status = send_outgoing_rpc(p0, p1, p2, p3, gateway, UVISOR_WAIT_FOREVER, &cookie);
    } while (status);
    msg_slot = uvisor_result_slot(cookie);

    /* Loop until sending the RPC message succeeds. */
    do {
        /* We also (because this is the sync function) wait forever for a result. */
        status = wait_for_rpc_result(msg_slot, UVISOR_WAIT_FOREVER);
    } while (status);

    /* This message result is valid now, because we woke up with a non-fatal
     * status. */
    result_value = outgoing_message_array()[msg_slot].result;

    free_outgoing_msg(msg_slot);

    return result_value;
}

/* Start an asynchronous RPC. After this call successfully completes, the
 * caller can, at any time in any thread, wait on the result object to get the
 * result of the call. */
uvisor_rpc_result_t rpc_fncall_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, const TRPCGateway * gateway)
{
    int status;
    uvisor_rpc_result_t cookie;

    /* Don't wait any length of time for an outgoing message slot. If there is
     * no slot available, return immediately with a non-zero status. */
    status = send_outgoing_rpc(p0, p1, p2, p3, gateway, 0, &cookie);
    if (status) {
        return status;
    }

    return cookie;
}

int rpc_fncall_wait(uvisor_rpc_result_t result, uint32_t timeout_ms, uint32_t * ret)
{
    int status;
    uvisor_pool_slot_t const msg_slot = uvisor_result_slot(result);
    uvisor_rpc_result_t const invalid = uvisor_result_build(UVISOR_RESULT_INVALID_COUNTER, msg_slot);

    /* If the cookie is invalid, this message is already being waited for.
     * Otherwise this is the first wait and we can proceed. */
    uvisor_rpc_result_t cookie = __sync_val_compare_and_swap(
            &outgoing_message_array()[msg_slot].wait_cookie,
            result,
            invalid);
    if (cookie != result) {
        return -1;
    }

    status = wait_for_rpc_result(msg_slot, timeout_ms);

    if (status) {
        return status;
    }

    /* The message result is valid now, because we woke up with a non-fatal
     * status. */
    *ret = outgoing_message_array()[msg_slot].result;

    free_outgoing_msg(msg_slot);

    return 0;
}

static uvisor_rpc_fn_group_t * allocate_function_group(const TFN_Ptr fn_ptr_array[], size_t fn_count)
{
    uvisor_pool_slot_t slot;
    uvisor_rpc_fn_group_t * fn_group;

    static const uint32_t timeout_ms = 0; /* Don't wait for an available slot. */
    slot = uvisor_pool_queue_allocate(fn_group_queue(), timeout_ms);

    if (slot >= fn_group_queue()->pool->num) {
        /* Nothing left in pool */
        return NULL;
    }

    fn_group = &fn_group_array()[slot];

    fn_group->fn_ptr_array = fn_ptr_array;
    fn_group->fn_count = fn_count;

    uvisor_pool_queue_enqueue(fn_group_queue(), slot);

    return fn_group;
}

/* Return a pointer to the function group specified by the fn_ptr_array, or
 * NULL if that function group isn't found. */
static uvisor_rpc_fn_group_t * get_function_group(const TFN_Ptr fn_ptr_array[], size_t fn_count)
{
    uvisor_pool_slot_t i;

    /* Find the entry with linear search */
    for (i = 0; i < fn_group_queue()->pool->num; i++) {
        /* If found: */
        if (fn_group_array()[i].fn_ptr_array == fn_ptr_array) {
            /* Return the target. */
            return &fn_group_array()[i];
        }
    }

    /* The entry for the given function pointer wasn't found. */
    return NULL;
}

/* Private structure for function group queries */
typedef struct query_for_fn_group_context {
    const TFN_Ptr *fn_ptr_array;
    size_t fn_count;
} query_for_fn_group_context_t;

static int query_for_fn_group(uvisor_pool_slot_t slot, void * context)
{
    query_for_fn_group_context_t * query_context = context;
    uvisor_rpc_message_t * msg = &incoming_message_array()[slot];
    size_t i;

    /* See if the target is for a function we can handle.
     * FIXME Optimize this search for large numbers of target functions per
     * box. */
    for (i = 0; i < query_context->fn_count; ++i) {
        if ((TFN_Ptr) msg->gateway->target == query_context->fn_ptr_array[i]) {
            /* Yes, we can handle this function call. */
            return 1;
        }
    }

    /* The message wasn't for a function we can handle. */
    return 0;
}

/* Return true if and only if we handled an RPC message */
static int handle_incoming_rpc(uvisor_rpc_fn_group_t * fn_group, int * box_id_caller) {
    uvisor_pool_slot_t msg_slot;
    uvisor_rpc_message_t * msg;
    query_for_fn_group_context_t context;

    context.fn_ptr_array = fn_group->fn_ptr_array;
    context.fn_count = fn_group->fn_count;

    msg_slot = uvisor_pool_queue_find_first(incoming_message_todo_queue(), query_for_fn_group, &context);
    if (msg_slot >= incoming_message_todo_queue()->pool->num) {
        /* We woke up for no reason. */
        return 0;
    }

    /* Dequeue the message */
    /* TODO Combine finding and dequeing into one operation, so we don't have any
     * chance to fail dequeing after we find a slot we are interested in. */
    msg_slot = uvisor_pool_queue_dequeue(incoming_message_todo_queue(), msg_slot);
    if (msg_slot >= incoming_message_todo_queue()->pool->num)
    {
        /* In between finding an RPC we could handle and trying to dequeue it,
         * somebody else dequeued it. */
        return 0;
    }

    msg = &incoming_message_array()[msg_slot];

    /* Verify the RPC gateway magic, for pure paranoia and detecting bugs
     * reasons. */
    if (!(msg->gateway->magic == UVISOR_RPC_GATEWAY_MAGIC_ASYNC ||
          msg->gateway->magic == UVISOR_RPC_GATEWAY_MAGIC_SYNC)) {
        /* The gateway was not valid, so we aren't going to handle this RPC
         * message. */
        return 0;
    }

    /* Save the calling box ID before the RPC target function is called, so
     * that the RPC target function can read that variable to find out what box
     * called it. */
    if (box_id_caller) {
        *box_id_caller = msg->other_box_id;
    }

    /* Dispatch the RPC. */
    TFN_Ptr fn = (TFN_Ptr) msg->gateway->target;
    msg->result = fn(msg->p0, msg->p1, msg->p2, msg->p3);

    msg->state = UVISOR_RPC_MESSAGE_STATE_DONE;

    /* Send the result back to the caller. */
    uvisor_pool_queue_enqueue(incoming_message_done_queue(), msg_slot);

    return 1;
}

int rpc_fncall_waitfor(const TFN_Ptr fn_ptr_array[], size_t fn_count, int * box_id_caller, uint32_t timeout_ms)
{
    uvisor_rpc_fn_group_t * fn_group;
    int status;
    int handled;

    /* Look up the semaphore to see if it exists already. */
    fn_group = get_function_group(fn_ptr_array, fn_count);

    /* If the function group doesn't exist yet: */
    if (fn_group == NULL) {
        /* Allocate a new entry for the function group. */
        fn_group = allocate_function_group(fn_ptr_array, fn_count);

        /* If we couldn't allocate a function group: */
        if (fn_group == NULL) {
            /* Too many function groups have already been allocated. */
            return UVISOR_ERROR_OUT_OF_STRUCTURES;
        }

        /* There may already be incoming messages we can handle, enqueued
         * before our function group was allocated. */
        handled = handle_incoming_rpc(fn_group, box_id_caller);
        if (handled) {
            /* We handled a message, so return. We don't want the first (or
             * any) call to rpc_fncall_waitfor handle more than incoming RPC
             * message. */
            return 0;
        }
        /* If there was nothing for us to handle yet, go ahead and wait on the
         * semaphore with the user-provided timeout. Don't return here saying
         * we didn't handle anything, because we want to obey the user-provided
         * timeout for all calls to rpc_fncall_waitfor. */
    }

    /* Wait for incoming RPC. */
    status = __uvisor_semaphore_pend(&fn_group->semaphore, timeout_ms);
    if (status) {
        /* The semaphore pend failed. */
        return status;
    }

    /* We woke up. Look for any RPC we can handle. */
    handled = handle_incoming_rpc(fn_group, box_id_caller);

    return handled == 1 ? 0 : -1;
}
