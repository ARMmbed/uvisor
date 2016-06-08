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
#include "uvisor-lib/uvisor-lib.h"
#include "cmsis_os.h"

extern UvisorBoxIndex *const __uvisor_ps;

static void thread_switch(void *context)
{
    if (context == NULL) return;

    /* If the active_heap is NULL, then the process heap needs to be
     * initialized yet. The initializer sets the active heap itself. */
    if (__uvisor_ps->active_heap) {
        __uvisor_ps->active_heap = context;
    }
}

const TUvisorExportTable __uvisor_export_table = {
    .magic = UVISOR_EXPORT_MAGIC,
    .version = UVISOR_EXPORT_VERSION,
    .thread_observer = {
        .version = 0,
        .thread_create = NULL,
        .thread_destroy = NULL,
        .thread_switch = thread_switch,
    },
    .size = sizeof(TUvisorExportTable)
};
