/*
 * Copyright (c) 2015, ARM Limited, All Rights Reserved
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
#include "mbed/mbed.h"
#include "uvisor-lib/uvisor-lib.h"

void uvisor_error(THaltUserError reason)
{
    register uint32_t __r0 __asm__("r0") = (uint32_t) reason;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_HALT_USER_ERR)
    );
}
