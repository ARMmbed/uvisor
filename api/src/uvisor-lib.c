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
#include "api/inc/halt_exports.h"
#include "rt_OsEventObserver.h"

int uvisor_lib_init(void)
{
    TUvisorExportTable const * const export_table = uvisor_export_table();

    if (export_table->magic != UVISOR_EXPORT_MAGIC) {
        /* We couldn't find the magic. */
        return UVISOR_ERROR_BAD_MAGIC;
    }

    if (export_table->version != UVISOR_EXPORT_VERSION) {
        /* The version we understand is not the version we found. */
        return UVISOR_ERROR_BAD_VERSION;
    }

    /* osRegisterForOsEvents won't allow a second call. For systems that don't
     * make use of osRegisterForOsEvents we recommend to
     * osRegisterForOsEvents(NULL) to disable further registrations (which if
     * allowed would be a backdoor). */
    osRegisterForOsEvents(&export_table->os_event_observer);

    return 0;
}
