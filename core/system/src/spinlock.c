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
#include "spinlock.h"

void spin_init(UvisorSpinlock * spinlock)
{
    __sync_synchronize();
    spinlock->acquired = false;
}

bool spin_trylock(UvisorSpinlock * spinlock)
{
    return __sync_bool_compare_and_swap(&spinlock->acquired, false, true);
}

void spin_lock(UvisorSpinlock * spinlock)
{
    while (spin_trylock(spinlock) == false);
}

void spin_unlock(UvisorSpinlock * spinlock)
{
    __sync_synchronize();
    spinlock->acquired = false;
}
