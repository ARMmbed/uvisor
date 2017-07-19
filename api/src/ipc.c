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
#include "api/inc/ipc.h"
#include "api/inc/ipc_exports.h"
#include "api/inc/halt_exports.h"
#include "api/inc/linker_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/uvisor_spinlock_exports.h"
#include "api/inc/vmpu_exports.h"
#include <string.h>
#include <assert.h>

extern UvisorBoxIndex * const __uvisor_ps;

static UvisorSpinlock * ipc_tokens_lock(void)
{
    return &uvisor_ipc(__uvisor_ps)->tokens_lock;
}

static uint32_t * ipc_allocated_tokens(void)
{
    return &uvisor_ipc(__uvisor_ps)->allocated_tokens;
}

static uint32_t * ipc_completed_tokens(void)
{
    return &uvisor_ipc(__uvisor_ps)->completed_tokens;
}

static uvisor_pool_queue_t * ipc_send_queue(void)
{
    return &uvisor_ipc(__uvisor_ps)->send_queue.queue;
}

static uvisor_ipc_io_t * ipc_send_array(void)
{
    return uvisor_ipc(__uvisor_ps)->send_queue.io;
}

static uvisor_pool_queue_t * ipc_recv_queue(void)
{
    return &uvisor_ipc(__uvisor_ps)->recv_queue.queue;
}

static uvisor_ipc_io_t * ipc_recv_array(void)
{
    return uvisor_ipc(__uvisor_ps)->recv_queue.io;
}

/** Allocate a token according to the position in the ipc queue
 * @param tokens[in,out] allocate the token slot from tokens, atomically modifying tokens
 * @param slot           slot number inside the queue
 * @param is_sending     whether the slot number is from sending queue
 * @return               the token, or 0 if token already allocated */
static uint32_t ipc_set_token_bit(uint32_t * tokens, uvisor_pool_slot_t slot, bool is_sending)
{
    /* TODO: Make this a static assert */
    assert(UVISOR_IPC_SEND_SLOTS + UVISOR_IPC_RECV_SLOTS == 32);
    uvisor_spin_lock(ipc_tokens_lock());
    /* Higher 16 bits for receiving, Lower 16 bits for sending */
    uint32_t token = 1U << (slot + (is_sending ? 0 : UVISOR_IPC_SEND_SLOTS));
    if (*tokens | token) {
        /* Already allocated */
        uvisor_spin_unlock(ipc_tokens_lock());
        return 0;
    }
    *tokens |= token; /* Allocate the token. */
    uvisor_spin_unlock(ipc_tokens_lock());
    return token;
}

/** Free tokens
 * @param    tokens[in,out] free the specified tokens_to_free from tokens, atomically modifying tokens
 * @param    tokens_to_free[in] the tokens to free (represented by 1 bits)
 * @return   the token, or 0 if no token available */
void ipc_free_tokens(uint32_t * tokens, uint32_t tokens_to_free)
{
    *tokens &= ~tokens_to_free;
}

static void ipc_free_allocated_completed_tokens(uint32_t tokens)
{
    ipc_free_tokens(ipc_allocated_tokens(), tokens);
    ipc_free_tokens(ipc_completed_tokens(), tokens);
}

static int ipc_waitfor(int (*cond)(uint32_t have, uint32_t expect), uint32_t wait_tokens, uint32_t * done_tokens, uint32_t timeout_ms)
{
    uint32_t completed_tokens;
    bool condition_met;
    for (;;) {
        /* Read the current tokens and clear any we were waiting on. */
        uvisor_spin_lock(ipc_tokens_lock());
        completed_tokens = *ipc_completed_tokens() & wait_tokens;
        condition_met = cond(completed_tokens, wait_tokens);
        if (condition_met) {
            /* Clear the tokens we were waiting on. */
            ipc_free_allocated_completed_tokens(completed_tokens);
        }
        uvisor_spin_unlock(ipc_tokens_lock());

        if (condition_met) {
            break;
        }

        /* FIXME Non-zero timeout not implemented for now. */
        if (timeout_ms == 0) {
            return UVISOR_ERROR_TIMEOUT;
        }
    }

    /* Communicate which tokens actually finished. */
    *done_tokens = completed_tokens;

    return 0;
}

/* We have at least one of what we expect. */
static int any(uint32_t have, uint32_t expect)
{
    return have & expect;
}

/* We have at least all of what we expect (and maybe more). */
static int all(uint32_t have, uint32_t expect)
{
    return (have & expect) == expect;
}

int ipc_waitforany(uint32_t wait_tokens, uint32_t * done_tokens, uint32_t timeout_ms) {
    return ipc_waitfor(any, wait_tokens, done_tokens, timeout_ms);
}

int ipc_waitforall(uint32_t wait_tokens, uint32_t * done_tokens, uint32_t timeout_ms)
{
    return ipc_waitfor(all, wait_tokens, done_tokens, timeout_ms);
}

static int ipc_io(uvisor_ipc_desc_t * desc, const void * msg, bool is_sending)
{
    uvisor_pool_queue_t * queue = is_sending ? ipc_send_queue() : ipc_recv_queue();
    uvisor_ipc_io_t * array = is_sending ? ipc_send_array() : ipc_recv_array();
    uvisor_pool_slot_t slot;
    uvisor_ipc_io_t * io;
    uint32_t new_token;

    /* Claim a slot in the IPC queue. */
    slot = uvisor_pool_queue_allocate(queue);
    uvisor_pool_t * pool = UVISOR_GET_NS_ALIAS(queue->pool);
    if (slot >= pool->num) {
        /* No slots are available in the queue. We asked for a free slot but
         * didn't get one. */
        return UVISOR_ERROR_OUT_OF_STRUCTURES;
    }

    /* Set the correspondent token according to slot allocated */
    new_token = ipc_set_token_bit(ipc_allocated_tokens(), slot, is_sending);
    if (!new_token) {
        /* This should not happen. This occurring means the pool allocates a
         * slot that has already been allocated */
        assert(false);
    }

    /* The message will be sent at this point. Communicate the allocated token
     * through the descriptor. */
    desc->token = new_token;

    /* Populate the IPC IO request. */
    io = UVISOR_GET_NS_ALIAS(&array[slot]);
    io->desc = desc;
    io->msg = (void *) msg;
    /* New requests will have READY state */
    io->state = UVISOR_IPC_IO_STATE_READY;

    /* Place the IPC request into the outgoing queue. */
    uvisor_pool_queue_enqueue(queue, slot);

    return 0;
}

int ipc_send(uvisor_ipc_desc_t * desc, const void * msg)
{
    return ipc_io(desc, msg, true);
}

int ipc_recv(uvisor_ipc_desc_t * desc, void * msg)
{
    return ipc_io(desc, msg, false);
}
