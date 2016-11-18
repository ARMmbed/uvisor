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
#include <uvisor.h>
#include "api/inc/api.h"
#include "box_init.h"
#include "debug.h"
#include "halt.h"
#include "svc.h"
#include "unvic.h"
#include "vmpu.h"
#include "page_allocator.h"
#include "thread.h"
#include "box_init.h"

#define transition_np_to_p(fn_target, fn_ret, fn_name, ...) \
    UVISOR_NAKED static \
    fn_ret fn_target ## _transition(__VA_ARGS__) { \
        asm volatile( \
            "svc %[svc_id] \n" \
            "bx lr         \n" \
            :: [svc_id] "I" (UVISOR_SVC_ID_GET(fn_target)) \
        ); \
        __builtin_unreachable(); \
    }

#define transition_p_to_p(fn_target, fn_ret, fn_name, ...) \
    UVISOR_NAKED static \
    fn_ret fn_target ## _transition(__VA_ARGS__) { \
        asm volatile( \
            "ldr r12, =" #fn_name "\n" \
            "bx r12                \n" \
        ); \
        __builtin_unreachable(); \
    }


transition_np_to_p(debug_init,       void, debug_register_driver, const TUvisorDebugDriver * const driver);
transition_np_to_p(irq_system_reset, void, debug_reboot,          TResetReason reason);
transition_np_to_p(error,            void, halt_user_error,       THaltUserError reason);

transition_np_to_p(vmpu_mem_invalidate, void, vmpu_mem_invalidate, void);

transition_np_to_p(box_namespace, int,  vmpu_box_namespace_from_id, int box_id, char * box_namespace, size_t length);

transition_np_to_p(page_malloc, int,  page_allocator_malloc,       UvisorPageTable * const table);
transition_np_to_p(page_free,   int,  page_allocator_free,   const UvisorPageTable * const table);

transition_np_to_p(irq_set_vector,    void,     unvic_isr_set,          uint32_t irqn, uint32_t vector);
transition_np_to_p(irq_get_vector,    uint32_t, unvic_isr_get,          uint32_t irqn);
transition_np_to_p(irq_enable,        void,     unvic_irq_enable,       uint32_t irqn);
transition_np_to_p(irq_disable,       void,     unvic_irq_disable,      uint32_t irqn);
transition_np_to_p(irq_clear_pending, void,     unvic_irq_pending_clr,  uint32_t irqn);
transition_np_to_p(irq_set_pending,   void,     unvic_irq_pending_set,  uint32_t irqn);
transition_np_to_p(irq_get_pending,   uint32_t, unvic_irq_pending_get,  uint32_t irqn);
transition_np_to_p(irq_set_priority,  void,     unvic_irq_priority_set, uint32_t irqn, uint32_t priority);
transition_np_to_p(irq_get_priority,  uint32_t, unvic_irq_priority_get, uint32_t irqn);
transition_np_to_p(irq_get_level,     int,      unvic_irq_level_get,    void);
transition_np_to_p(irq_disable_all,   void,     unvic_irq_disable_all,  void);
transition_np_to_p(irq_enable_all,    void,     unvic_irq_enable_all,   void);

transition_p_to_p(pre_start,       void,   boxes_init, void);
transition_p_to_p(thread_create,   void *, thread_create, int id, void * c);
transition_p_to_p(thread_destroy,  void,   thread_destroy, void * c);
transition_p_to_p(thread_switch,   void,   thread_switch, void * c);

static uint32_t get_api_version(uint32_t version)
{
    /* Ignore expected version for now, can be used later for compatibility. */
    (void) version;
    return UVISOR_API_VERSION;
}

__attribute__((section(".uvisor_api")))
const UvisorApi __uvisor_api = {
    .magic = UVISOR_API_MAGIC,
    .get_version = get_api_version,

    .init = main_entry,

    .irq_enable = irq_enable_transition,
    .irq_disable = irq_disable_transition,
    .irq_disable_all = irq_disable_all_transition,
    .irq_enable_all = irq_enable_all_transition,
    .irq_set_vector = irq_set_vector_transition,
    .irq_get_vector = irq_get_vector_transition,
    .irq_set_pending = irq_set_pending_transition,
    .irq_get_pending = irq_get_pending_transition,
    .irq_clear_pending = irq_clear_pending_transition,
    .irq_set_priority = irq_set_priority_transition,
    .irq_get_priority = irq_get_priority_transition,
    .irq_get_level = irq_get_level_transition,
    .irq_system_reset = irq_system_reset_transition,

    .page_malloc = page_malloc_transition,
    .page_free = page_free_transition,

    .box_namespace = box_namespace_transition,

    .debug_init = debug_init_transition,
    .error = error_transition,

    .vmpu_mem_invalidate = vmpu_mem_invalidate_transition,

    .pool_init = uvisor_pool_init,
    .pool_queue_init = uvisor_pool_queue_init,
    .pool_allocate = uvisor_pool_allocate,
    .pool_free = uvisor_pool_free,
    .pool_queue_enqueue = uvisor_pool_queue_enqueue,
    .pool_queue_dequeue = uvisor_pool_queue_dequeue,
    .pool_queue_dequeue_first = uvisor_pool_queue_dequeue_first,
    .pool_queue_find_first = uvisor_pool_queue_find_first,

    .os_event_observer = {
        .version = 0,
        .pre_start = pre_start_transition,
        .thread_create = thread_create_transition,
        .thread_destroy = thread_destroy_transition,
        .thread_switch = thread_switch_transition,
    },
};
