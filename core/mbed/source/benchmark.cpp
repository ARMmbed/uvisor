/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
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

void uvisor_benchmark_configure(void)
{
    /* in sequence:
     *   - configure the DWT (default overhead = 0)
     *   - execute a nop measurement to assess the overhead introduced by the
     *     measurement itself
     *   - pass the computed overhead to the configuration routine for future
     *     measurements */
    __asm__ volatile(
        "push {lr}\n"
        "mov  r0, #0\n"
        "svc  %[svc_id_benchmark_cfg]\n"
        "bl   uvisor_benchmark_start\n"
        "bl   uvisor_benchmark_stop\n"
        "svc  %[svc_id_benchmark_cfg]\n"
        "pop  {pc}\n"
        :: [svc_id_benchmark_cfg]  "i" (UVISOR_SVC_ID_BENCHMARK_CFG)
         : "r0"
    );
}

void uvisor_benchmark_start(void)
{
    __asm__ volatile(
        "svc %[svc_id_benchmark_rst]\n"
        :: [svc_id_benchmark_rst] "i" (UVISOR_SVC_ID_BENCHMARK_RST)
    );
}

uint32_t uvisor_benchmark_stop(void)
{
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id_benchmark_stop]\n"
        : [res]                   "=r" (__res)
        : [svc_id_benchmark_stop] "i"  (UVISOR_SVC_ID_BENCHMARK_STOP)
    );
    return __res;
}
