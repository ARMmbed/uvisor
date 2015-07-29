/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __BOX_CONFIG_H__
#define __BOX_CONFIG_H__

UVISOR_EXTERN const uint32_t __uvisor_mode;

#define UVISOR_DISABLED   0
#define UVISOR_PERMISSIVE 1
#define UVISOR_ENABLED    2

#define UVISOR_SET_MODE(mode) \
    UVISOR_SET_MODE_ACL_COUNT(mode, NULL, 0)

#define UVISOR_SET_MODE_ACL(mode, acl_list) \
    UVISOR_SET_MODE_ACL_COUNT(mode, acl_list, UVISOR_ARRAY_COUNT(acl_list))

#define UVISOR_SET_MODE_ACL_COUNT(mode, acl_list, acl_list_count) \
    uint8_t __attribute__((section(".uvisor.bss.stack"), aligned(32))) \
        __reserved_stack[UVISOR_STACK_BAND_SIZE]; \
    \
    UVISOR_EXTERN const uint32_t __uvisor_mode = (mode); \
    \
    static UVISOR_SECURE_CONST UvisorBoxConfig main_cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        0, \
        0, \
        acl_list, \
        acl_list_count \
    }; \
    \
    const __attribute__((section(".uvisor.cfgtbl_first"), aligned(4))) \
          volatile void* main_cfg_ptr  =  & main_cfg;

#define UVISOR_SECURE_CONST \
    const volatile __attribute__((section(".uvisor.secure"), aligned(32)))

#define UVISOR_SECURE_BSS \
    __attribute__((section(".uvisor.bss.data"), aligned(32)))

#define UVISOR_SECURE_DATA \
    __attribute__((section(".uvisor.data"), aligned(32)))

/* this macro selects an overloaded macro (variable number of arguments) */
#define __UVISOR_BOX_MACRO(_1, _2, _3, _4, NAME, ...) NAME

#define __UVISOR_BOX_CONFIG(box_name, acl_list, stack_size, context_size) \
    \
    uint8_t __attribute__((section(".uvisor.bss.stack"), aligned(32))) \
        box_name ## _reserved[UVISOR_STACK_SIZE_ROUND(((UVISOR_MIN_STACK(stack_size) + (context_size))*8)/6)]; \
    \
    static UVISOR_SECURE_CONST UvisorBoxConfig box_name ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        UVISOR_MIN_STACK(stack_size), \
        context_size, \
        acl_list, \
        UVISOR_ARRAY_COUNT(acl_list) \
    }; \
    \
    const __attribute__((section(".uvisor.cfgtbl"), aligned(4))) \
          volatile void *box_name ## _cfg_ptr  =  & box_name ## _cfg;

#define __UVISOR_BOX_CONFIG_NOCONTEXT(box_name, acl_list, stack_size) \
    __UVISOR_BOX_CONFIG(box_name, acl_list, stack_size, 0) \

#define __UVISOR_BOX_CONFIG_CONTEXT(box_name, acl_list, stack_size, context_type) \
    __UVISOR_BOX_CONFIG(box_name, acl_list, stack_size, sizeof(context_type)) \
    UVISOR_EXTERN context_type * const uvisor_ctx;

#define UVISOR_BOX_CONFIG(...) \
    __UVISOR_BOX_MACRO(__VA_ARGS__, __UVISOR_BOX_CONFIG_CONTEXT, \
                                    __UVISOR_BOX_CONFIG_NOCONTEXT)(__VA_ARGS__)

#endif/*__BOX_CONFIG_H__*/
