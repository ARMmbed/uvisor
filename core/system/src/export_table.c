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
#include "api/inc/pool_queue_exports.h"
#include "box_init.h"
#include "thread.h"

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
