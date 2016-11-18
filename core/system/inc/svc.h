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
#ifndef __SVC_H__
#define __SVC_H__

#include "api/inc/svc_exports.h"

#define UVISOR_SVC_ID_GET(target) UVISOR_SVC_CUSTOM_TABLE(offsetof(UvisorSvcTarget, target) / sizeof(uint32_t))

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

    void (*debug_init)(const TUvisorDebugDriver * const driver);
    void (*error)(THaltUserError reason);
    void (*vmpu_mem_invalidate)(void);
} UVISOR_PACKED UvisorSvcTarget;

/* macro to execute an SVCall; additional metadata can be provided, which will
 * be appended right after the svc instruction */
/* note: the macro is implicitly overloaded to allow 0 to 4 32bits arguments */
#define UVISOR_SVC(id, metadata, ...) \
    ({ \
        UVISOR_MACRO_REGS_ARGS(uint32_t, ##__VA_ARGS__); \
        UVISOR_MACRO_REGS_RETVAL(uint32_t, res); \
        asm volatile( \
            "svc %[svc_id]\n" \
            metadata \
            : UVISOR_MACRO_GCC_ASM_OUTPUT(res) \
            : UVISOR_MACRO_GCC_ASM_INPUT(__VA_ARGS__), \
              [svc_id] "I" ((id) & 0xFF) \
        ); \
        res; \
    })

#define UVISOR_FUNCTION_CALL(dst_fn, ...) \
    ({ \
        UVISOR_MACRO_REGS_ARGS(uint32_t, ##__VA_ARGS__); \
        UVISOR_MACRO_REGS_RETVAL(uint32_t, res); \
        asm volatile( \
            "bl " UVISOR_TO_STRING(dst_fn) "\n" \
            : UVISOR_MACRO_GCC_ASM_OUTPUT(res) \
            : UVISOR_MACRO_GCC_ASM_INPUT(__VA_ARGS__) \
        ); \
        res; \
    })

void svc_init(void);
void __svc_not_implemented(void);

#endif/*__SVC_H__*/
