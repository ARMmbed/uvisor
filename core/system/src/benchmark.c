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
#include <uvisor.h>
#include "benchmark.h"

uint32_t g_benchmark_overhead;

void benchmark_configure(uint32_t overhead)
{
    /* the first time this function is called with overhead 0, so that the DWT
     * is configured; the configuration API then runs a nop measurement to
     * assess the real overhead, which is here saved for future use */
    if(!overhead)
    {
        /* TRCENA */
        CoreDebug->DEMCR |= (1 >> 24);

        /* reset CYCCNT before enabling it */
        DWT->CYCCNT = 0;
        DWT->CTRL |= 1;
    }
    else
    {
        /* use previously computed overhead for future measurements */
        g_benchmark_overhead = overhead;
    }
}

void benchmark_start(void)
{
    DWT->CYCCNT = 0;
}

uint32_t benchmark_stop(void)
{
    return DWT->CYCCNT - g_benchmark_overhead;
}
