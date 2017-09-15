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

/** Allocate a token
 * @param    tokens[in,out] allocate an available token from tokens, atomically modifying tokens
 * @return   the token, or 0 if no token available */
uint32_t ipc_allocate_token(uint32_t * tokens)
{
    uint32_t token;
    uint32_t bit;

    /* Read the current tokens and allocate a new token. Both reading and
     * allocating must be done while holding the spin lock to prevent two (or
     * more) concurrent allocations from grabbing the same token. */
    uvisor_spin_lock(ipc_tokens_lock());
    /* FIXME Optimize, to lessen the time holding the spin lock. */
    bit = 0x1;
    while (*tokens & bit) {
        bit <<= 1;
    }
    token = bit;
    *tokens |= token; /* Allocate the token. */
    uvisor_spin_unlock(ipc_tokens_lock());

    /* NOTE: This function depends on UVISOR_IPC_INVALID_TOKEN being defined to
     * 0. This is because if no bits are available in the bitfield, this
     * function returns 0. In order for the user of this API to interpret this
     * failure correctly, UVISOR_IPC_INVALID_TOKEN must be 0. */
    UVISOR_STATIC_ASSERT(UVISOR_IPC_INVALID_TOKEN == 0, UVISOR_IPC_INVALID_TOKEN_should_be_zero);
    return token;
}

/** Free tokens
 *  The token lock must have been already acquired
 * @param    tokens[in,out] free the specified tokens_to_free from tokens, atomically modifying tokens
 * @param    tokens_to_free[in] the tokens to free (represented by bits) */
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
        uvisor_spin_lock(ipc_tokens_lock());
        /* Check we are not waiting for some unallocated tokens */
        if ((wait_tokens & *ipc_allocated_tokens()) != wait_tokens) {
            uvisor_spin_unlock(ipc_tokens_lock());
            return UVISOR_ERROR_INVALID_PARAMETERS;
        }
        /* Read the current tokens and clear any we were waiting on. */
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
    return (expect == 0) || have & expect;
}

/* We have at least all of what we expect (and maybe more). */
static int all(uint32_t have, uint32_t expect)
{
    return (have & expect) == expect;
}

int ipc_waitforany(uint32_t wait_tokens, uint32_t * done_tokens, uint32_t timeout_ms)
{
    return ipc_waitfor(any, wait_tokens, done_tokens, timeout_ms);
}

int ipc_waitforall(uint32_t wait_tokens, uint32_t * done_tokens, uint32_t timeout_ms)
{
    return ipc_waitfor(all, wait_tokens, done_tokens, timeout_ms);
}

static int ipc_io(uvisor_ipc_desc_t * desc, const void * msg,
                  uvisor_pool_queue_t * queue, uvisor_ipc_io_t * array, uvisor_ipc_io_state_t new_state)
{
    uvisor_pool_slot_t slot;
    uvisor_ipc_io_t * io;
    uint32_t new_token;

    /* Allocate a token. If none left, err. */
    new_token = ipc_allocate_token(ipc_allocated_tokens());
    if (new_token == UVISOR_IPC_INVALID_TOKEN) {
        return UVISOR_ERROR_OUT_OF_STRUCTURES;
    }

    /* Claim a slot in the IPC queue. */
    slot = uvisor_pool_queue_allocate(queue);
    uvisor_pool_t * pool = UVISOR_GET_NS_ALIAS(queue->pool);
    if (slot >= pool->num) {
        /* No slots are available in the queue. We asked for a free slot but
         * didn't get one. */
        return UVISOR_ERROR_OUT_OF_STRUCTURES;
    }

    /* The message will be sent at this point. Communicate the allocated token
     * through the descriptor. */
    desc->token = new_token;

    /* Populate the IPC IO request. */
    io = UVISOR_GET_NS_ALIAS(&array[slot]);
    io->desc = desc;
    io->msg = (void *) msg;
    io->state = new_state;

    /* Place the IPC request into the outgoing queue. */
    if (slot != uvisor_pool_queue_enqueue(queue, slot)) {
        /* Enqueue failed */
        return UVISOR_ERROR_OUT_OF_STRUCTURES;
    }

    return 0;
}

int ipc_send(uvisor_ipc_desc_t * desc, const void * msg)
{
    return ipc_io(desc, msg, ipc_send_queue(), ipc_send_array(), UVISOR_IPC_IO_STATE_READY_TO_SEND);
}

int ipc_recv(uvisor_ipc_desc_t * desc, void * msg)
{
    return ipc_io(desc, msg, ipc_recv_queue(), ipc_recv_array(), UVISOR_IPC_IO_STATE_READY_TO_RECV);
}
