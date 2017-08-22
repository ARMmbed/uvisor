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
#include "api/inc/linker_exports.h"
#include "api/inc/pool_queue_exports.h"
#include "api/inc/uvisor_spinlock_exports.h"
#include <stddef.h>
#include <string.h>

int uvisor_pool_init(uvisor_pool_t * pool, void * array, size_t stride, size_t num)
{
    uvisor_pool_slot_t i;

    if (num == 0 || num >= UVISOR_POOL_MAX_VALID) {
        return -1;
    }

    pool->magic = UVISOR_POOL_MAGIC;

    /* Force saving of NS alias, so no matter where the pool queue is init, the
     * NS side doesn't have to adjust the address to access. */
    pool->array = UVISOR_GET_NS_ALIAS(array);

    pool->stride = stride;
    pool->num = num;
    pool->num_allocated = 0;
    pool->first_free = 0;

    for (i = 0; i < num; i++) {
        pool->management_array[i].dequeued.next = i + 1;
        pool->management_array[i].dequeued.state = UVISOR_POOL_SLOT_IS_FREE;
    }
    pool->management_array[num - 1].dequeued.next = UVISOR_POOL_SLOT_INVALID;

    uvisor_spin_init(&pool->spinlock);

    UVISOR_STATIC_ASSERT(
            sizeof(uvisor_pool_t) == offsetof(uvisor_pool_t, management_array),
            management_array_offset_must_be_aligned_to_pool_structure_size);

    return 0;
}

int uvisor_pool_queue_init(uvisor_pool_queue_t * pool_queue, uvisor_pool_t * pool, void * array, size_t stride, size_t num)
{
    int pool_init_ret = uvisor_pool_init(pool, array, stride, num);
    if (pool_init_ret) {
        return pool_init_ret;
    }
    pool_queue->magic = UVISOR_POOL_QUEUE_MAGIC;
    pool_queue->head = UVISOR_POOL_SLOT_INVALID;
    pool_queue->tail = UVISOR_POOL_SLOT_INVALID;

    /* Force saving of NS alias, so no matter where the pool queue is init, the
     * NS side doesn't have to adjust the address to access. */
    pool_queue->pool = UVISOR_GET_NS_ALIAS(pool);

    return 0;
}

/* Remove an element from the front of the free list. */
static uvisor_pool_slot_t pool_alloc(uvisor_pool_t * pool)
{
    uvisor_pool_slot_t fresh = pool->first_free;
    if (pool->first_free != UVISOR_POOL_SLOT_INVALID) {
        uvisor_pool_queue_entry_t * first_free;
        first_free = &pool->management_array[pool->first_free];

        pool->first_free = first_free->dequeued.next;

        /* This assignment isn't strictly necessary, but for debugging
         * purposes, it may help us find bugs sooner. */
        pool->management_array[fresh].dequeued.next = UVISOR_POOL_SLOT_INVALID;

        /* This assignment is necessary still, as it helps prevents double
         * frees from the pool. */
        pool->management_array[fresh].dequeued.state = UVISOR_POOL_SLOT_IS_DEQUEUED;

        ++pool->num_allocated;
    }

    return fresh;
}

/* Add an element to the back of the queue. */
static uvisor_pool_slot_t enqueue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);
    if (slot >= pool->num) {
        /* Reject out of bound access */
        return UVISOR_POOL_SLOT_INVALID;
    }
    uvisor_pool_queue_entry_t * slot_entry = &pool->management_array[slot];

    if (slot_entry->dequeued.state == UVISOR_POOL_SLOT_IS_FREE) {
        /* Reject if a free slot is being enqueued */
        return UVISOR_POOL_SLOT_IS_FREE;
    }
    if (slot_entry->queued.prev < pool->num
            || slot_entry->queued.prev == UVISOR_POOL_SLOT_INVALID) {
        /* Reject enqueuing a slot twice */
        return UVISOR_POOL_SLOT_INVALID;
    }
    if (slot_entry->queued.prev < pool->num
            || slot_entry->queued.prev == UVISOR_POOL_SLOT_INVALID) {
        /* Reject enqueuing a slot twice */
        return -1;
    }

    /* If this is the first allocated slot. */
    if (pool_queue->head == UVISOR_POOL_SLOT_INVALID) {
        pool_queue->head = slot;
        uvisor_pool_queue_entry_t * head_entry = &pool->management_array[pool_queue->head];
        head_entry->queued.prev = UVISOR_POOL_SLOT_INVALID;
    } else {
        /* This is not the first allocated slot, so we have to update where
         * tail's prev next points to. */
        uvisor_pool_queue_entry_t * tail_entry = &pool->management_array[pool_queue->tail];
        tail_entry->queued.next = slot;
        slot_entry->queued.prev = pool_queue->tail;
    }

    /* Add slot to the end of the queue. */
    slot_entry->queued.next = UVISOR_POOL_SLOT_INVALID;
    pool_queue->tail = slot;

    return slot;
}

uvisor_pool_slot_t uvisor_pool_allocate(uvisor_pool_t * pool)
{
    /* uvisor should try lock. users should wait forever... */
    uvisor_spin_lock(&pool->spinlock);
    uvisor_pool_slot_t fresh = pool_alloc(pool);
    uvisor_spin_unlock(&pool->spinlock);

    return fresh;
}

uvisor_pool_slot_t uvisor_pool_try_allocate(uvisor_pool_t * pool)
{
    /* uvisor should try lock. users should wait forever... */
    bool locked = uvisor_spin_trylock(&pool->spinlock);
    if (!locked) {
        /* We didn't get the lock. */
        return UVISOR_POOL_SLOT_INVALID;
    }
    uvisor_pool_slot_t fresh = pool_alloc(pool);
    uvisor_spin_unlock(&pool->spinlock);

    return fresh;
}

uvisor_pool_slot_t uvisor_pool_queue_enqueue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);
    uvisor_pool_slot_t ret = UVISOR_POOL_SLOT_INVALID;

    if (slot != UVISOR_POOL_SLOT_INVALID) {
        uvisor_spin_lock(&pool->spinlock);
        ret = enqueue(pool_queue, slot);
        uvisor_spin_unlock(&pool->spinlock);
    }
    return ret;
}

uvisor_pool_slot_t uvisor_pool_queue_try_enqueue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);
    uvisor_pool_slot_t ret = UVISOR_POOL_SLOT_INVALID;

    if (slot != UVISOR_POOL_SLOT_INVALID) {
        bool locked = uvisor_spin_trylock(&pool->spinlock);
        if (!locked) {
            /* We couldn't lock. */
            return ret;
        }
        ret = enqueue(pool_queue, slot);
        uvisor_spin_unlock(&pool->spinlock);
    }

    return ret;
}

static uvisor_pool_slot_t pool_free(uvisor_pool_t * pool, uvisor_pool_slot_t slot)
{
    uvisor_pool_queue_entry_t * slot_entry = &pool->management_array[slot];
    uvisor_pool_slot_t state = slot_entry->dequeued.state;
    if (state == UVISOR_POOL_SLOT_IS_FREE) {
        /* Already freed. Return. */
        return state;
    } else if (state == UVISOR_POOL_SLOT_IS_DEQUEUED) {
        /* Slot to free is a dequeued slot. Free the slot */
        slot_entry->dequeued.next = pool->first_free;
        slot_entry->dequeued.state = UVISOR_POOL_SLOT_IS_FREE;

        pool->first_free = slot;

        --pool->num_allocated;

        return slot;
    } else {
        /* If state of the queue is not free or dequeued, the slot must have
         * been used as an enqueued slot. In this case, freeing the slot will
         * lead to corruption of the data structure. Reject. */
        return UVISOR_POOL_SLOT_INVALID;
    }

}

static uvisor_pool_slot_t dequeue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);
    if (slot >= pool->num) {
        /* Reject out of bound access */
        return UVISOR_POOL_SLOT_INVALID;
    }

    uvisor_pool_queue_entry_t * slot_entry = &pool->management_array[slot];
    uvisor_pool_slot_t state = slot_entry->dequeued.state;
    if (state == UVISOR_POOL_SLOT_IS_FREE || state == UVISOR_POOL_SLOT_IS_DEQUEUED) {
        /* Return the state if the slot is not in the queue */
        return state;
    }

    if (pool_queue->head == UVISOR_POOL_SLOT_INVALID) {
        /* Reject dequeueing when queue is empty*/
        return UVISOR_POOL_SLOT_INVALID;
    }

    /* Dequeue the slot */
    if (pool_queue->head == slot) {
        pool_queue->head = slot_entry->queued.next;
    } else {
        uvisor_pool_queue_entry_t * prev_entry = &pool->management_array[slot_entry->queued.prev];
        prev_entry->queued.next = slot_entry->queued.next;
    }
    if (pool_queue->tail == slot) {
        pool_queue->tail = slot_entry->queued.prev;
    } else {
        uvisor_pool_queue_entry_t * next_entry = &pool->management_array[slot_entry->queued.next];
        next_entry->queued.prev = slot_entry->queued.prev;
    }

    slot_entry->dequeued.next = UVISOR_POOL_SLOT_INVALID;
    slot_entry->dequeued.state = UVISOR_POOL_SLOT_IS_DEQUEUED;

    return slot;
}

uvisor_pool_slot_t uvisor_pool_free(uvisor_pool_t * pool, uvisor_pool_slot_t slot)
{
    /* TODO refactor with uvisor_pool_queue_dequeue */
    if (slot >= pool->num) {
        return UVISOR_POOL_SLOT_INVALID;
    }

    uvisor_spin_lock(&pool->spinlock);
    slot = pool_free(pool, slot);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_try_free(uvisor_pool_t * pool, uvisor_pool_slot_t slot)
{
    /* TODO refactor with uvisor_pool_queue_dequeue */
    if (slot >= pool->num) {
        return UVISOR_POOL_SLOT_INVALID;
    }

    bool locked = uvisor_spin_trylock(&pool->spinlock);
    if (!locked) {
        /* We couldn't get the lock. */
        return UVISOR_POOL_SLOT_INVALID;
    }
    slot = pool_free(pool, slot);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_queue_try_dequeue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    /* TODO refactor with uvisor_pool_free */
    if (slot >= pool->num) {
        return UVISOR_POOL_SLOT_INVALID;
    }

    bool locked = uvisor_spin_trylock(&pool->spinlock);
    if (!locked) {
        /* We didn't get the lock. */
        return UVISOR_POOL_SLOT_INVALID;
    }
    slot = dequeue(pool_queue, slot);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_queue_dequeue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    /* TODO refactor with uvisor_pool_free */
    if (slot >= pool->num) {
        return UVISOR_POOL_SLOT_INVALID;
    }

    uvisor_spin_lock(&pool->spinlock);
    slot = dequeue(pool_queue, slot);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_queue_dequeue_first(uvisor_pool_queue_t * pool_queue)
{
    uvisor_pool_slot_t slot;
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    uvisor_spin_lock(&pool->spinlock);
    slot = dequeue(pool_queue, pool_queue->head);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_queue_try_dequeue_first(uvisor_pool_queue_t * pool_queue)
{
    uvisor_pool_slot_t slot;
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    bool locked = uvisor_spin_trylock(&pool->spinlock);
    if (!locked) {
        /* We didn't get the lock. */
        return UVISOR_POOL_SLOT_INVALID;
    }
    slot = dequeue(pool_queue, pool_queue->head);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

static uvisor_pool_slot_t find_first(uvisor_pool_queue_t * pool_queue,
                                     TQueryFN_Ptr query_fn, void * context)
{
    uvisor_pool_slot_t slot;
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);
    uvisor_pool_slot_t iterated = 0;

    /* Walk the queue, looking for the first slot that matches the query. */
    slot = pool_queue->head;
    while (slot != UVISOR_POOL_SLOT_INVALID && iterated < UVISOR_POOL_MAX_VALID)
    {
        uvisor_pool_queue_entry_t * entry = &pool->management_array[slot];

        /* NOTE: The query function is called with the queue spinlock held, so
         * be careful. */
        int query_result = query_fn(slot, context);

        if (query_result) {
            return slot;
        }

        slot = entry->queued.next;
        if (slot >= pool->num) {
            /* Queue corrupted or end of queue */
            break;
        }
        iterated++;
    }

    /* We didn't find a match. */
    return UVISOR_POOL_SLOT_INVALID;
}

uvisor_pool_slot_t uvisor_pool_queue_try_find_first(uvisor_pool_queue_t * pool_queue,
                                                    TQueryFN_Ptr query_fn, void * context)
{
    uvisor_pool_slot_t slot;
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    bool locked = uvisor_spin_trylock(&pool->spinlock);
    if (!locked) {
        /* We didn't get the lock. */
        return UVISOR_POOL_SLOT_INVALID;
    }
    slot = find_first(pool_queue, query_fn, context);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}

uvisor_pool_slot_t uvisor_pool_queue_find_first(uvisor_pool_queue_t * pool_queue,
                                                TQueryFN_Ptr query_fn, void * context)
{
    uvisor_pool_slot_t slot;
    uvisor_pool_t * pool = UVISOR_AUTO_NS_ALIAS(pool_queue->pool);

    uvisor_spin_lock(&pool->spinlock);
    slot = find_first(pool_queue, query_fn, context);
    uvisor_spin_unlock(&pool->spinlock);

    return slot;
}
