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
#ifndef __UVISOR_LIB_BOX_CONFIG_H__
#define __UVISOR_LIB_BOX_CONFIG_H__

#include <stddef.h>
#include <stdbool.h>

UVISOR_EXTERN const uint32_t __uvisor_mode;

/* In an effort to hide the box-local variables that
 * uvisor_buffer_overlaps_box_context needs from colliding with other boxes,
 * and to avoid defining uvisor_buffer_overlaps_box_context inside a macro that
 * would make uvisor_buffer_overlaps_box_context itself box-local, we privately
 * predeclare those box-local variables as private. The exact mechanism of
 * privacy differs between C and C++. */
#ifdef __cplusplus
/* In C++, we can't predeclare variables that will later be defined as static.
 * So, we instead define the variables as extern, but in a private anonymous
 * namespace. */
namespace
{
    extern const uintptr_t __uvisor_ctx_addr;
    extern const size_t __uvisor_ctx_size;
}
#else
/* In C, we can predeclare variables that will later be defined as static. */
const size_t __uvisor_ctx_size;
const uintptr_t __uvisor_ctx_addr;
#endif

#ifdef __cplusplus
/* In C++, we define the private variables that
 * uvisor_buffer_overlaps_box_context needs inside a private anonymous
 * namespace. */
#define __UVISOR_DEFINE_CTX_SIZE(box_name) \
    namespace \
    { \
        const size_t __uvisor_ctx_size = sizeof(box_name ## _reserved); \
    }

#define __UVISOR_DEFINE_CTX_ADDR \
    namespace \
    { \
        const uintptr_t __uvisor_ctx_addr = (uintptr_t) &uvisor_ctx; \
    }
#else
/* In C, we define the private variables that
 * uvisor_buffer_overlaps_box_context needs as static. */
#define __UVISOR_DEFINE_CTX_SIZE(box_name) \
    static const size_t __uvisor_ctx_size = sizeof(box_name ## _reserved);

#define __UVISOR_DEFINE_CTX_ADDR(box_name) \
    static const size_t __uvisor_ctx_addr = &uvisor_ctx;
#endif

#define UVISOR_DISABLED   0
#define UVISOR_PERMISSIVE 1
#define UVISOR_ENABLED    2

/* The maximum box namespace length is 37 so that it is exactly big enough for
 * a human-readable hex string GUID (as formatted by RFC 4122) followed by a
 * terminating NUL. */
#define UVISOR_MAX_BOX_NAMESPACE_LENGTH 37

#define UVISOR_SET_MODE(mode) \
    UVISOR_SET_MODE_ACL_COUNT(mode, NULL, 0)

#define UVISOR_SET_MODE_ACL(mode, acl_list) \
    UVISOR_SET_MODE_ACL_COUNT(mode, acl_list, UVISOR_ARRAY_COUNT(acl_list))

#define UVISOR_SET_MODE_ACL_COUNT(mode, acl_list, acl_list_count) \
    uint8_t __attribute__((section(".keep.uvisor.bss.boxes"), aligned(32))) __reserved_stack[UVISOR_STACK_BAND_SIZE]; \
    \
    UVISOR_EXTERN const uint32_t __uvisor_mode = (mode); \
    \
    static const __attribute__((section(".keep.uvisor.cfgtbl"), aligned(4))) UvisorBoxConfig main_cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        0, \
        0, \
        NULL, \
        acl_list, \
        acl_list_count \
    }; \
    \
    extern const __attribute__((section(".keep.uvisor.cfgtbl_ptr_first"), aligned(4))) void * const main_cfg_ptr = &main_cfg;

/* this macro selects an overloaded macro (variable number of arguments) */
#define __UVISOR_BOX_MACRO(_1, _2, _3, _4, NAME, ...) NAME

#define __UVISOR_BOX_CONFIG(box_name, acl_list, acl_list_count, stack_size, context_size) \
    \
    uint8_t __attribute__((section(".keep.uvisor.bss.boxes"), aligned(32))) \
        box_name ## _reserved[UVISOR_STACK_SIZE_ROUND(((UVISOR_MIN_STACK(stack_size) + (context_size))*8)/6)]; \
    \
    static const __attribute__((section(".keep.uvisor.cfgtbl"), aligned(4))) UvisorBoxConfig box_name ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        UVISOR_MIN_STACK(stack_size), \
        context_size, \
        __uvisor_box_namespace, \
        acl_list, \
        acl_list_count \
    }; \
    \
    extern const __attribute__((section(".keep.uvisor.cfgtbl_ptr"), aligned(4))) void * const box_name ## _cfg_ptr = &box_name ## _cfg; \
    __UVISOR_DEFINE_CTX_SIZE(box_name)

#define __UVISOR_BOX_CONFIG_NOCONTEXT(box_name, acl_list, stack_size) \
    __UVISOR_BOX_CONFIG(box_name, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, 0) \

#define __UVISOR_DECLARE_CTX(context_type) \
    UVISOR_EXTERN context_type * const uvisor_ctx; \
    __UVISOR_DEFINE_CTX_ADDR

#define __UVISOR_BOX_CONFIG_CONTEXT(box_name, acl_list, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_name, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, sizeof(context_type)) \
    __UVISOR_DECLARE_CTX(context_type)

#define __UVISOR_BOX_CONFIG_NOACL(box_name, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_name, NULL, 0, stack_size, sizeof(context_type)) \
    __UVISOR_DECLARE_CTX(context_type)

#define __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT(box_name, stack_size) \
    __UVISOR_BOX_CONFIG(box_name, NULL, 0, stack_size, 0)

#define UVISOR_BOX_CONFIG_ACL(...) \
    __UVISOR_BOX_MACRO(__VA_ARGS__, __UVISOR_BOX_CONFIG_CONTEXT, \
                                    __UVISOR_BOX_CONFIG_NOCONTEXT, \
                                    __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT)(__VA_ARGS__)

#define UVISOR_BOX_CONFIG_CTX(...) \
    __UVISOR_BOX_MACRO(__VA_ARGS__, __UVISOR_BOX_CONFIG_CONTEXT, \
                                    __UVISOR_BOX_CONFIG_NOACL, \
                                    __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT)(__VA_ARGS__)

#define UVISOR_BOX_CONFIG(...) \
    UVISOR_BOX_CONFIG_ACL(__VA_ARGS__)

/* Use this macro before box defintion (for example, UVISOR_BOX_CONFIG) to
 * define the name of your box. If you don't want a name, use this macro with
 * box_namespace as NULL. */
#define UVISOR_BOX_NAMESPACE(box_namespace) \
    static const char *const __uvisor_box_namespace = box_namespace


/* Return true if (and only if) the specified buffer overlaps box context. */
static inline bool uvisor_buffer_overlaps_box_context(const void *buffer, size_t length)
{
    uintptr_t buffer_addr = (uintptr_t) buffer;

    return (buffer_addr <= __uvisor_ctx_addr &&
            buffer_addr + length > __uvisor_ctx_addr) ||
           (__uvisor_ctx_addr <= buffer_addr &&
            __uvisor_ctx_addr + __uvisor_ctx_size > buffer_addr);
}

/* Return the numeric box ID of the current box. */
UVISOR_EXTERN int uvisor_box_id_self(void);

/* Return the numeric box ID of the box that is calling through the most recent
 * secure gateway. Return -1 if there is no secure gateway calling box. */
UVISOR_EXTERN int uvisor_box_id_caller(void);

/* Copy the box namespace of the specified box ID to the memory provided by
 * box_namespace. The box_namespace's length must be at least
 * MAX_BOX_NAMESPACE_LENGTH bytes. Return how many bytes were copied into
 * box_namespace. Return UVISOR_ERROR_INVALID_BOX_ID if the provided box ID is
 * invalid. Return UVISOR_ERROR_BUFFER_TOO_SMALL if the provided box_namespace
 * is too small to hold MAX_BOX_NAMESPACE_LENGTH bytes. Return
 * UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS if the box is anonymous. */
UVISOR_EXTERN int uvisor_box_namespace(int box_id, char *box_namespace, size_t length);

#endif /* __UVISOR_LIB_BOX_CONFIG_H__ */
