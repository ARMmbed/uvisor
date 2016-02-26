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

UVISOR_EXTERN const uint32_t __uvisor_mode;

#define UVISOR_DISABLED   0
#define UVISOR_PERMISSIVE 1
#define UVISOR_ENABLED    2

#define UVISOR_MAX_BOX_NAME_LENGTH 41

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
#define __UVISOR_BOX_MACRO_5(_1, _2, _3, _4, _5, NAME, ...) NAME

#define __UVISOR_BOX_CONFIG(box_prefix, box_name, acl_list, acl_list_count, stack_size, context_size) \
    \
    uint8_t __attribute__((section(".keep.uvisor.bss.boxes"), aligned(32))) \
        box_prefix ## _reserved[UVISOR_STACK_SIZE_ROUND(((UVISOR_MIN_STACK(stack_size) + (context_size))*8)/6)]; \
    \
    static const __attribute__((section(".keep.uvisor.cfgtbl"), aligned(4))) UvisorBoxConfig box_prefix ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        UVISOR_MIN_STACK(stack_size), \
        context_size, \
        box_name, \
        acl_list, \
        acl_list_count \
    }; \
    \
    extern const __attribute__((section(".keep.uvisor.cfgtbl_ptr"), aligned(4))) void * const box_prefix ## _cfg_ptr = &box_prefix ## _cfg;

#define __UVISOR_BOX_CONFIG_NOCONTEXT(box_prefix, acl_list, stack_size) \
    __UVISOR_BOX_CONFIG(box_prefix, NULL, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, 0) \

#define __UVISOR_BOX_CONFIG_CONTEXT(box_prefix, acl_list, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_prefix, NULL, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, sizeof(context_type)) \
    UVISOR_EXTERN context_type * const uvisor_ctx;

#define __UVISOR_BOX_CONFIG_NOACL(box_prefix, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_prefix, NULL, NULL, 0, stack_size, sizeof(context_type)) \
    UVISOR_EXTERN context_type * const uvisor_ctx;

#define __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT(box_prefix, stack_size) \
    __UVISOR_BOX_CONFIG(box_prefix, NULL, NULL, 0, stack_size, 0)

#define __UVISOR_BOX_CONFIG_NOCONTEXT_NAME(box_prefix, box_name, acl_list, stack_size) \
    __UVISOR_BOX_CONFIG(box_prefix, box_name, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, 0) \

#define __UVISOR_BOX_CONFIG_CONTEXT_NAME(box_prefix, box_name, acl_list, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_prefix, box_name, acl_list, UVISOR_ARRAY_COUNT(acl_list), stack_size, sizeof(context_type)) \
    UVISOR_EXTERN context_type * const uvisor_ctx;

#define __UVISOR_BOX_CONFIG_NOACL_NAME(box_prefix, box_name, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_prefix, box_name, NULL, 0, stack_size, sizeof(context_type)) \
    UVISOR_EXTERN context_type * const uvisor_ctx;

#define __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT_NAME(box_prefix, box_name, stack_size) \
    __UVISOR_BOX_CONFIG(box_prefix, box_name, NULL, 0, stack_size, 0)

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

#define UVISOR_BOX_CONFIG_ACL_NAME(...) \
    __UVISOR_BOX_MACRO_5(__VA_ARGS__, __UVISOR_BOX_CONFIG_CONTEXT_NAME, \
                                      __UVISOR_BOX_CONFIG_NOCONTEXT_NAME, \
                                      __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT_NAME)(__VA_ARGS__)

#define UVISOR_BOX_CONFIG_CTX_NAME(...) \
    __UVISOR_BOX_MACRO_5(__VA_ARGS__, __UVISOR_BOX_CONFIG_CONTEXT_NAME, \
                                      __UVISOR_BOX_CONFIG_NOACL_NAME, \
                                      __UVISOR_BOX_CONFIG_NOACL_NOCONTEXT_NAME)(__VA_ARGS__)

#define UVISOR_BOX_CONFIG_NAME(...) \
    UVISOR_BOX_CONFIG_ACL_NAME(__VA_ARGS__)

/* Return the numeric box ID of the current box. */
UVISOR_EXTERN int uvisor_box_id_self(void);

/* Return the numeric box ID of the box that is calling through the most recent
 * secure gateway. Return -1 if there is no secure gateway calling box. */
UVISOR_EXTERN int uvisor_box_id_caller(void);

/* Copy the box name of the specified box ID to the memory provided by
 * box_name. The box_name's length must be at least MAX_BOX_NAME_LENGTH bytes.
 * Return how many bytes were copied into box_name. Return -1 if the provided
 * box_name is too small to hold MAX_BOX_NAME_LENGTH bytes. */
UVISOR_EXTERN int uvisor_box_name(uint8_t box_id, char *box_name, size_t length);

#endif /* __UVISOR_LIB_BOX_CONFIG_H__ */
