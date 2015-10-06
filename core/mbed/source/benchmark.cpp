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
#include "uvisor-lib/uvisor-lib.h"

void uvisor_benchmark_configure(void)
{
    /* in sequence:
     *   - initialize the benchmarking unit (default overhead = 0)
     *   - execute a nop measurement to assess the overhead introduced by the
     *     measurement itself
     *   - pass the computed overhead to the configuration routine to complete
     *     the calibration */
    UVISOR_SVC(UVISOR_SVC_ID_BENCHMARK_CFG, "", 0);
    uvisor_benchmark_start();
    UVISOR_SVC(UVISOR_SVC_ID_BENCHMARK_CFG, "", uvisor_benchmark_stop());
}

void uvisor_benchmark_start(void)
{
    UVISOR_SVC(UVISOR_SVC_ID_BENCHMARK_RST, "");
}

uint32_t uvisor_benchmark_stop(void)
{
    return UVISOR_SVC(UVISOR_SVC_ID_BENCHMARK_STOP, "");
}
