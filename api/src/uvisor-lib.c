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
#include "api/inc/export_table_exports.h"
#include "api/inc/box_mains.h"

static TUvisorExportTable * __uvisor_export_table;

int uvisor_lib_init(void)
{
    /* Supported */
    /* Defined in uvisor-input.S */
    extern uint32_t uvisor_config;
    extern uint32_t uvisor_export_table_size;

    uintptr_t uvisor_config_addr = (uintptr_t) &uvisor_config;

    __uvisor_export_table = (TUvisorExportTable *) (uvisor_config_addr - uvisor_export_table_size);

    if (__uvisor_export_table->magic != UVISOR_EXPORT_MAGIC) {
        /* We couldn't find the magic. */
        return -1;
    }

    if (__uvisor_export_table->version != UVISOR_EXPORT_VERSION) {
        /* The version we understand is not the version we found. */
        return -1;
    }

    extern void osRegisterForOsEvents(const OsEventObserver *observer); /* XXX Use rt_OsEventObserver instead. */
    osRegisterForOsEvents(&__uvisor_export_table->os_event_observer);

    return 0;
}

/* This is called before the OS is started, but after the OS is initialized. */
int uvisor_lib_init_post(void)
{
    /* XXX TODO Assert that export table is non-0 */

    /* Start all the box main threads. */
    box_mains_start(__uvisor_export_table->set_thread_creation_context);

    return 0;
}
