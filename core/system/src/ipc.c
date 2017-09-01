/*
 * Copyright (c) 2017, ARM Limited, All Rights Reserved
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
#include "api/inc/ipc_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/uvisor_spinlock_exports.h"
#include "api/inc/vmpu_exports.h"
#include "context.h"
#include "halt.h"
#include "ipc.h"
#include "linker.h"
#include "vmpu.h"
#include "vmpu_mpu.h"
#include <string.h>

static UvisorBoxIndex * box_index(uint8_t box_id)
{
    return (UvisorBoxIndex *) g_context_current_states[box_id].bss;
}

typedef struct recv_match_context {
    int send_box_id;
    uvisor_ipc_io_t * send_io;
    uvisor_ipc_io_t * recv_array;
} recv_match_context_t;

static int recv_match(uvisor_pool_slot_t slot, void * context)
{
    recv_match_context_t * c = (recv_match_context_t *) context;
    uvisor_ipc_io_t * send_io = c->send_io;
    uvisor_ipc_io_t * recv_io = &c->recv_array[slot];
    uvisor_ipc_desc_t * send_desc = send_io->desc;
    uvisor_ipc_desc_t * recv_desc = recv_io->desc;
    int send_box_id = c->send_box_id;

    /* Ready to receive? */
    if (recv_io->state != UVISOR_IPC_IO_STATE_READY_TO_RECV) {
        return 0; /* The receive IO is not ready */
    }

    /* Source box ID permitted, or any box permitted to send? */
    if (send_box_id != recv_desc->box_id)
    {
        /* Source box ID was not permitted explicitly. Is any box permitted to
         * fulfil with receive request? */
        if (recv_desc->box_id != UVISOR_BOX_ID_ANY) {
            /* No. The ID must explicitly match and it doesn't match. */
            return 0;
        }
    }

    /* Port match? */
    if (send_desc->port != recv_desc->port) {
        return 0; /* Port doesn't match; fail matching. */
    }

    /* Enough room? */
    if (send_desc->len > recv_desc->len) {
        return 0; /* The sender is trying to send too large of a message. */
    }


    /* FIXME It would be nice to notify the sender with an error of why the
     * message delivery failed. Think about how to provide that information.
     * All we have now is that there was no recv request matching the send
     * request, but it'd be nice to have more detail if possible.
     *
     * A possible communication channel between uVisor and the application
     * would be the IPC descriptor. If we move "state" out of the IO object and
     * into the descriptor, the state could reflect error cases. Alternatively,
     * we can put a new error field into the descriptor. */

    /* This recv IO matches the send IO, so we can send the message to this
     * slot. */
    return 1;
}

static int put_it_back(uvisor_pool_queue_t * queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_slot_t status;
    status = uvisor_pool_queue_try_enqueue(queue, slot);
    if (status != slot) {
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

     return 0;
}

/* Fulfil the IPC request pair. Copy and update the descriptors. Clear the
 * tokens. Return 0 on success, non-zero otherwise.
 *
 * This function assumes that the relevant objects and buffers involved have
 * been checked and found properly accessible by the relevant boxes.
 * */
static int ipc_deliver(uvisor_ipc_t * send_ipc, uvisor_ipc_t * recv_ipc,
                       uvisor_ipc_io_t * send_io, uvisor_ipc_io_t * recv_io,
                       int send_box_id)
{
    int status = -1;
    uvisor_ipc_desc_t * send_desc = send_io->desc;
    uvisor_ipc_desc_t * recv_desc = recv_io->desc;
    UvisorSpinlock * send_lock = &send_ipc->tokens_lock;
    UvisorSpinlock * recv_lock = &recv_ipc->tokens_lock;
    void * recv_msg = UVISOR_GET_S_ALIAS(recv_io->msg);
    void * send_msg = UVISOR_GET_S_ALIAS(send_io->msg);

    /* We have to hold both the send and recv token locks when delivering the
     * message so that we don't ever partially deliver a message. The locks are
     * needed to mark transactions as complete, and we shouldn't copy anything
     * into the IO objects until we know we can safely mark the transactions as
     * complete. */
    /* FIXME Use a read/write lock so that uVisor can block out readers while
     * updating tokens. We don't really want token readers to block out uVisor
     * from delivering messages. */
    if (!uvisor_spin_trylock(send_lock)) {
        goto done;
    }

    if (recv_lock != send_lock && !uvisor_spin_trylock(recv_lock)) {
        goto unlock_send;
    }

    size_t len = send_desc->len;
    memmove(recv_msg, send_msg, len);
    send_ipc->completed_tokens |= send_desc->token;

    recv_desc->box_id = send_box_id;
    recv_desc->len = send_desc->len;
    recv_ipc->completed_tokens |= recv_desc->token;

    status = 0;

    if (recv_lock != send_lock) {
        uvisor_spin_unlock(recv_lock);
    }
unlock_send:
    uvisor_spin_unlock(send_lock);
done:
    return status;
}

static bool ipc_is_ok(int box_id, const uvisor_ipc_t * ipc) {
    return ipc &&
           vmpu_buffer_access_is_ok(box_id, ipc, sizeof(*ipc));
}

static bool ipc_io_array_is_ok(int box_id, const uvisor_ipc_io_t * array) {
    UVISOR_STATIC_ASSERT(UVISOR_IPC_SEND_SLOTS == UVISOR_IPC_RECV_SLOTS, UVISOR_IPC_SEND_SLOTS_should_be_equal_to_UVISOR_IPC_RECV_SLOTS);

    return array &&
           vmpu_buffer_access_is_ok(box_id, array, sizeof(*array) * UVISOR_IPC_SEND_SLOTS);
}

static bool pool_queue_is_ok(int box_id, const uvisor_pool_queue_t * queue) {
    return queue &&
           vmpu_buffer_access_is_ok(box_id, queue, sizeof(*queue)) &&
           queue->pool &&
           vmpu_buffer_access_is_ok(box_id, queue->pool, sizeof(*queue->pool)) &&
           vmpu_buffer_access_is_ok(box_id, queue->pool->array, queue->pool->stride * queue->pool->num);
}

static bool ipc_io_is_ok(int box_id, const uvisor_ipc_io_t * io) {
    return io &&
           vmpu_buffer_access_is_ok(box_id, io, sizeof(*io)) &&
           io->desc &&
           vmpu_buffer_access_is_ok(box_id, io->desc, sizeof(*(io->desc))) &&
           vmpu_buffer_access_is_ok(box_id, io->msg, io->desc->len);
}

void ipc_drain_queue(void)
{
    uint8_t send_box_id = g_active_box;
    int first_slot = -1;

    /*
     * Verify that the send IPC structures are OK to use.
     */
    uvisor_ipc_t * send_ipc = UVISOR_GET_S_ALIAS(uvisor_ipc(box_index(send_box_id)));
    if (!ipc_is_ok(send_box_id, send_ipc)) {
        /* This shouldn't happen in a non-malicious box. */
        return;
    }

    uvisor_pool_queue_t * send_queue = &send_ipc->send_queue.queue;
    if (!pool_queue_is_ok(send_box_id, send_queue)) {
        /* This shouldn't happen in a non-malicious box. */
        return;
    }

    uvisor_ipc_io_t * send_array = send_ipc->send_queue.io;
    if (!ipc_io_array_is_ok(send_box_id, send_array)) {
        /* This shouldn't happen in a non-malicious box. */
        return;
    }

    /*
     * Loop over all outgoing messages for this box, looking for a matching
     * receive in a destination box. If any matches are found, deliver the
     * matching outgoing messages.
     */
    do {
        uvisor_pool_slot_t send_slot;
        uvisor_pool_slot_t recv_slot;

        /* Get the first item in the send queue. */
        send_slot = uvisor_pool_queue_try_dequeue_first(send_queue);
        if (send_slot >= send_queue->pool->num) {
            /* The queue is empty or busy. */
            break;
        }

        /* If we have seen this slot before, stop processing the queue. */
        if (first_slot == -1) {
            first_slot = send_slot;
        } else if (send_slot == first_slot) {
            put_it_back(send_queue, send_slot);

            /* Stop looping, because the system needs to continue running so
             * the recv messages can get processed to free up more room. */
            break;
        }

        /* Verify that the send IO request is OK to use. */
        uvisor_ipc_io_t * send_io = &send_array[send_slot];
        if (!ipc_io_is_ok(send_box_id, send_io)) {
            /* The IO is not entirely within the send box. Ignore it, and don't
             * put it back. This shouldn't happen in a non-malicious box. */
            continue;
        }

        uvisor_ipc_desc_t * send_desc = send_io->desc;

        /* Ready to send? */
        if (send_io->state != UVISOR_IPC_IO_STATE_READY_TO_SEND) {
            put_it_back(send_queue, send_slot);
            continue; /* Try the next message */
        }

        /* Look up the receiving box. */
        const int recv_box_id = send_desc->box_id;
        if (recv_box_id < 0 || recv_box_id >= g_vmpu_box_count) {
            /* Ignore messages sent to boxes we don't know. */
            continue;
        }

        /*
         * Verify that the receive IPC structures are OK to use.
         */
        uvisor_ipc_t * recv_ipc = UVISOR_GET_S_ALIAS(uvisor_ipc(box_index(recv_box_id)));
        if (!ipc_is_ok(recv_box_id, recv_ipc)) {
            /* This shouldn't happen in a non-malicious box. */
            put_it_back(send_queue, send_slot);
            continue; /* Try the next send IO. */
        }

        uvisor_pool_queue_t * recv_queue = &recv_ipc->recv_queue.queue;
        if (!pool_queue_is_ok(recv_box_id, recv_queue)) {
            /* This shouldn't happen in a non-malicious box. */
            put_it_back(send_queue, send_slot);
            continue; /* Try the next send IO. */
        }

        uvisor_ipc_io_t * recv_array = recv_ipc->recv_queue.io;
        if (!ipc_io_array_is_ok(recv_box_id, recv_array)) {
            /* This shouldn't happen in a non-malicious box. */
            put_it_back(send_queue, send_slot);
            continue; /* Try the next send IO. */
        }

        /* Find the first recv IO in the recv_queue that matches the port and
         * allows from this sender. */
        recv_match_context_t context = {send_box_id, send_io, recv_array};
        recv_slot = uvisor_pool_queue_try_find_first(recv_queue, recv_match, &context);
        /* Was a receive request available to match the send request? */
        if (recv_slot >= recv_queue->pool->num) {
            /* No recv request was available. Try the next send request. */
            put_it_back(send_queue, send_slot);
            continue;
        }

        recv_slot = uvisor_pool_queue_try_dequeue(recv_queue, recv_slot);
        if (recv_slot >= recv_queue->pool->num) {
            /* In between finding a recv slot and trying to dequeue it,
             * somebody else dequeued it. */
            put_it_back(send_queue, send_slot);
            continue;
        }

        /* We have a send and receive request pair. Do the copying and updating
         * of the descriptor, and clearing of the token. */
        uvisor_ipc_io_t * recv_io = &recv_array[recv_slot];
        if (!ipc_io_is_ok(recv_box_id, recv_io)) {
            /* The IO is not entirely within the send box. Ignore it, and don't
             * put it back. */
            put_it_back(send_queue, send_slot);
            continue;
        }

        if (ipc_deliver(send_ipc, recv_ipc, send_io, recv_io, send_box_id)) {
            /* The message couldn't be delivered at this time. */
            put_it_back(send_queue, send_slot);
            put_it_back(recv_queue, recv_slot);
            continue;
        }

        /* We were able to send a message, so reset the first slot counter if
         * we sent the first slot. */
        if (first_slot == send_slot) {
            first_slot = -1;
        }

#ifndef NDEBUG
        uvisor_ipc_desc_t * recv_desc = recv_io->desc;
#endif
        DPRINTF("Delivered [b%d:s%d].t0x%08x to [b%d:s%d].t0x%08x\r\n", send_box_id, send_slot, send_desc->token, recv_box_id, recv_slot, recv_desc->token);

        /* Free the slots, as we have consumed the IOs. */
        send_slot = uvisor_pool_queue_try_free(send_queue, send_slot);
        recv_slot = uvisor_pool_queue_try_free(recv_queue, recv_slot);
        if ((send_slot >= send_queue->pool->num) ||
            (recv_slot >= recv_queue->pool->num)) {
            /* The queue is empty or busy. This should never happen. We were
             * able to dequeue a result message, but weren't able to free the
             * result message. It is bad to take down the entire system. It is
             * also bad to never free slots in the outgoing result queue.
             * However, if we could dequeue the slot we should have no trouble
             * freeing the slot here. */
            assert(false);
        }
    } while (1);
}

void ipc_box_init(uint8_t box_id)
{
    uvisor_ipc_t * ipc = UVISOR_GET_S_ALIAS(uvisor_ipc(box_index(box_id)));
    uvisor_ipc_send_queue_t * send_queue = &ipc->send_queue;
    uvisor_ipc_recv_queue_t * recv_queue = &ipc->recv_queue;

    /* Initialize the IPC send queue. */
    if (uvisor_pool_queue_init(&send_queue->queue,
                               &send_queue->pool,
                               send_queue->io,
                               sizeof(*send_queue->io),
                               UVISOR_IPC_SEND_SLOTS)) {
        HALT_ERROR(NOT_ALLOWED, "Failed to init IPC send queue");
    }

    /* Initialize the IPC receive queue. */
    if (uvisor_pool_queue_init(&recv_queue->queue,
                               &recv_queue->pool,
                               recv_queue->io,
                               sizeof(*recv_queue->io),
                               UVISOR_IPC_RECV_SLOTS)) {
        HALT_ERROR(NOT_ALLOWED, "Failed to init IPC recv queue");
    }

    uvisor_spin_init(&ipc->tokens_lock);
    ipc->allocated_tokens = 0;
    ipc->completed_tokens = 0;
}
