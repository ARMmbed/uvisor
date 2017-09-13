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

/* Halt if uVisor is not loaded in a sane manner before calling any of its
 * APIs. `uvisor_init` is the first API called by convention, so perform the
 * check here. If this check is not performed, and uVisor is loaded insanely,
 * the call to uVisor's init function might take the CPU into la-la land. */
static void uvisor_sanity_check_api_load(void)
{
    /* The uVisor binary thinks that uvisor_api is loaded at
     * &__uvisor_main_start. Make sure this expectation is correct. */
    extern uint32_t __uvisor_main_start;
    UvisorApi *uvisor_bin_api = (UvisorApi *) &__uvisor_main_start;
    if (&uvisor_api != uvisor_bin_api ||
        uvisor_api.magic != UVISOR_API_MAGIC ||
        uvisor_bin_api->magic != UVISOR_API_MAGIC) {

        /* We can't print anything here, as printing will allocate memory (even
         * putchar) and uVisor wraps malloc. Memory allocation depends on the
         * uVisor API working. */

        /* We can't call uvisor_error either, because the API is messed up. */
        uvisor_noreturn();
    }
}

/* Halt if the uVisor API version is not what we expect. */
static void uvisor_sanity_check_api_version()
{
    if (uvisor_api.get_version(UVISOR_API_VERSION) != UVISOR_API_VERSION) {
        /* We can't call any uVisor APIs other than get_version (which is a
         * backwards and forwards compatible API), so we must halt. */
        uvisor_noreturn();
    }
}

/* Halt if the uVisor API is not loaded correctly or not of a compatible
 * version. */
static void uvisor_sanity_check_api() {
    uvisor_sanity_check_api_load();
    uvisor_sanity_check_api_version();
}

void uvisor_init(void)
{
    uvisor_sanity_check_api();

    uvisor_api.init((uint32_t) __builtin_return_address(0));
    /* FIXME: Function return does not clean up the stack! */
    __builtin_unreachable();
}

void uvisor_start(void)
{
    uvisor_api.start();
}

int uvisor_lib_init(void)
{
    /* osRegisterForOsEvents won't allow a second call. For systems that don't
     * make use of osRegisterForOsEvents we recommend to
     * osRegisterForOsEvents(NULL) to disable further registrations (which if
     * allowed would be a backdoor). */
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
    osRegisterForOsEvents(NULL); // On ARMv8-M, we don't need an OsEventObserver.
#else
    osRegisterForOsEvents(&uvisor_api.os_event_observer);
#endif

    extern void __uvisor_initialize_rpc_queues(void);
    __uvisor_initialize_rpc_queues();

    return 0;
}
