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
#include "api/inc/api.h"
#include "api/inc/halt_exports.h"
#include "rt_OsEventObserver.h"

void uvisor_init(void)
{
    uvisor_api.init((uint32_t) __builtin_return_address(0));
    /* FIXME: Function return does not clean up the stack! */
    __builtin_unreachable();
}

int uvisor_lib_init(void)
{
    /* osRegisterForOsEvents won't allow a second call. For systems that don't
     * make use of osRegisterForOsEvents we recommend to
     * osRegisterForOsEvents(NULL) to disable further registrations (which if
     * allowed would be a backdoor). */
    osRegisterForOsEvents(&uvisor_api.os_event_observer);

    /* FIXME: Disabled for v8-M: This calls SVCs before PSP is set (in KernelStart), which is bad! */
#if !defined (__ARM_FEATURE_CMSE)
    extern void __uvisor_initialize_rpc_queues(void);
    __uvisor_initialize_rpc_queues();
#endif

    return 0;
}
