/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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
#ifndef __SVC_v7M_H__
#define __SVC_v7M_H__

#include "api/inc/svc_exports.h"

typedef struct {
    void (*not_implemented)(void);

    void     (*irq_enable)(uint32_t irqn);
    void     (*irq_disable)(uint32_t irqn);
    void     (*irq_disable_all)(void);
    void     (*irq_enable_all)(void);
    void     (*irq_set_vector)(uint32_t irqn, uint32_t vector);
    uint32_t (*irq_get_vector)(uint32_t irqn);
    void     (*irq_set_priority)(uint32_t irqn, uint32_t priority);
    uint32_t (*irq_get_priority)(uint32_t irqn);
    void     (*irq_set_pending)(uint32_t irqn);
    uint32_t (*irq_get_pending)(uint32_t irqn);
    void     (*irq_clear_pending)(uint32_t irqn);
    int      (*irq_get_level)(void);
    void     (*irq_system_reset)(TResetReason reason);

    int (*page_malloc)(UvisorPageTable * const table);
    int (*page_free)(const UvisorPageTable * const table);

    int (*box_namespace)(int box_id, char *box_namespace, size_t length);
    int (*box_id_for_namespace)(int * const box_id, const char * const box_namespace);

    void (*error)(THaltUserError reason);
    void (*vmpu_mem_invalidate)(void);
    void (*debug_semihosting_enable)(void);
} UVISOR_PACKED UvisorSvcTarget;

#endif /* __SVC_v7M_H__ */
