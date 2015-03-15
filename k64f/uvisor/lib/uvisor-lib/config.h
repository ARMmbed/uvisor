/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __CONFIG_H__
#define __CONFIG_H__

UVISOR_EXTERN const uint32_t __uvisor_mode;

#define UVISOR_SET_MODE(mode) \
    uint8_t __attribute__((section(".uvisor.bss.stack"), aligned(32))) \
        __reserved_stack[UVISOR_STACK_BAND_SIZE]; \
    UVISOR_EXTERN const uint32_t __uvisor_mode = (mode);

#define UVISOR_SECURE_CONST \
    const volatile __attribute__((section(".uvisor.secure"), aligned(32)))

#define UVISOR_SECURE_BSS \
    __attribute__((section(".uvisor.bss.data"), aligned(32)))

#define UVISOR_SECURE_DATA \
    __attribute__((section(".uvisor.data"), aligned(32)))

#define UVISOR_BOX_CONFIG(box_name, acl_list, stack_size) \
    \
    uint8_t __attribute__((section(".uvisor.bss.stack"), aligned(32))) \
        box_name ## _stack[UVISOR_STACK_SIZE_ROUND(stack_size)];\
    \
    static UVISOR_SECURE_CONST UvisorBoxConfig box_name ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        sizeof(box_name ## _stack), \
        acl_list, \
        UVISOR_ARRAY_COUNT(acl_list) \
    }; \
    \
    const __attribute__((section(".uvisor.cfgtbl"), aligned(4))) \
          volatile void* box_name ## _cfg_ptr  =  & box_name ## _cfg;

#endif/*__CONFIG_H__*/
