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

#ifdef ARCH_CORE_ARMv8M

/* TODO Consider making the Makefile duplicate this as part of the build. */
#include "core/system/src/spinlock.c"

#else

#include "api/inc/uvisor_spinlock_exports.h"
#include "api/inc/api.h"

void uvisor_spin_init(UvisorSpinlock * spinlock)
{
    uvisor_api.spin_init(spinlock);
}

bool uvisor_spin_trylock(UvisorSpinlock * spinlock)
{
    return uvisor_api.spin_trylock(spinlock);
}

void uvisor_spin_lock(UvisorSpinlock * spinlock)
{
    uvisor_api.spin_lock(spinlock);
}

void uvisor_spin_unlock(UvisorSpinlock * spinlock)
{
    uvisor_api.spin_unlock(spinlock);
}

#endif
