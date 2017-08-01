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

#ifdef ARCH_CORE_ARMv8M

/* TODO Consider making the Makefile duplicate this as part of the build. */
#include "core/system/src/pool_queue.c"

#else

#include "api/inc/pool_queue_exports.h"
#include "api/inc/api.h"

int uvisor_pool_init(uvisor_pool_t * pool, void * array, size_t stride, size_t num)
{
    return uvisor_api.pool_init(pool, array, stride, num);
}

int uvisor_pool_queue_init(uvisor_pool_queue_t * pool_queue, uvisor_pool_t * pool, void * array, size_t stride, size_t num)
{
    return uvisor_api.pool_queue_init(pool_queue, pool, array, stride, num);
}

uvisor_pool_slot_t uvisor_pool_allocate(uvisor_pool_t * pool)
{
    return uvisor_api.pool_allocate(pool);
}

uvisor_pool_slot_t uvisor_pool_try_allocate(uvisor_pool_t * pool)
{
    return uvisor_api.pool_try_allocate(pool);
}

uvisor_pool_slot_t uvisor_pool_queue_enqueue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_queue_enqueue(pool_queue, slot);
}

uvisor_pool_slot_t uvisor_pool_queue_try_enqueue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_queue_try_enqueue(pool_queue, slot);
}

uvisor_pool_slot_t uvisor_pool_free(uvisor_pool_t * pool, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_free(pool, slot);
}

uvisor_pool_slot_t uvisor_pool_try_free(uvisor_pool_t * pool, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_try_free(pool, slot);
}

uvisor_pool_slot_t uvisor_pool_queue_dequeue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_queue_dequeue(pool_queue, slot);
}

uvisor_pool_slot_t uvisor_pool_queue_try_dequeue(uvisor_pool_queue_t * pool_queue, uvisor_pool_slot_t slot)
{
    return uvisor_api.pool_queue_try_dequeue(pool_queue, slot);
}

uvisor_pool_slot_t uvisor_pool_queue_dequeue_first(uvisor_pool_queue_t * pool_queue)
{
    return uvisor_api.pool_queue_dequeue_first(pool_queue);
}

uvisor_pool_slot_t uvisor_pool_queue_try_dequeue_first(uvisor_pool_queue_t * pool_queue)
{
    return uvisor_api.pool_queue_try_dequeue_first(pool_queue);
}

uvisor_pool_slot_t uvisor_pool_queue_find_first(uvisor_pool_queue_t * pool_queue, TQueryFN_Ptr query_fn, void * context)
{
    return uvisor_api.pool_queue_find_first(pool_queue,query_fn, context);
}

uvisor_pool_slot_t uvisor_pool_queue_try_find_first(uvisor_pool_queue_t * pool_queue, TQueryFN_Ptr query_fn, void * context)
{
    return uvisor_api.pool_queue_try_find_first(pool_queue, query_fn, context);
}

#endif
