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
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "api/inc/uvisor_spinlock_exports.h"
#include <stdbool.h>

/* This function is safe to call from interrupt context. */
void spin_init(UvisorSpinlock * spinlock);

/* Attempt to spin lock once. Return true if the lock was obtained, false if
 * otherwise. This function is safe to call from interrupt context. */
bool spin_trylock(UvisorSpinlock * spinlock);

/* Spin in a tight loop until the lock is obtained. This function is safe to
 * call from interrupt context, but probably not wise. */
void spin_lock(UvisorSpinlock * spinlock);

/* Unlock the spin lock. This function is safe to call from interrupt context.
 * */
void spin_unlock(UvisorSpinlock * spinlock);

#endif
