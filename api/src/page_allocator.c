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
#include "api/inc/uvisor-lib.h"
#include "core/uvisor.h"
#include <stdint.h>

int uvisor_page_malloc(UvisorPageTable * const table)
{
    return UVISOR_SVC(UVISOR_SVC_ID_PAGE_MALLOC, "", table);
}

int uvisor_page_free(const UvisorPageTable * const table)
{
    return UVISOR_SVC(UVISOR_SVC_ID_PAGE_FREE, "", table);
}
