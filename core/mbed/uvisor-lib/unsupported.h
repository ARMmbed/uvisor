/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2015-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __UNSUPPORTED_H__
#define __UNSUPPORTED_H__

/* uvisor hook for unsupported platforms */
UVISOR_EXTERN void uvisor_init(void);

/*******************************************************************************
 * re-definitions from:
 ******************************************************************************/

/* config.h */
UVISOR_EXTERN const uint32_t __uvisor_mode;

#define UVISOR_DISABLED 0

#define UVISOR_SET_MODE(mode) \
    UVISOR_SET_MODE_ACL_COUNT(mode, NULL, 0)

#define UVISOR_SET_MODE_ACL(mode, acl_list) \
    UVISOR_SET_MODE_ACL_COUNT(mode, \
                              acl_list, \
                              UVISOR_ARRAY_COUNT(acl_list))

#define UVISOR_SET_MODE_ACL_COUNT(mode, acl_list, acl_list_count) \
    UVISOR_EXTERN const uint32_t __uvisor_mode = UVISOR_DISABLED; \
    UVISOR_SECURE_CONST void *main_acl = acl_list;

#define UVISOR_SECURE_CONST const volatile
#define UVISOR_SECURE_BSS
#define UVISOR_SECURE_DATA

#define UVISOR_BOX_CONFIG(box_name, acl_list, stack_size) \
    UVISOR_SECURE_CONST void *box_acl_ ## box_name = acl_list;

/* interrupts.h */
#define vIRQ_SetVectorX(irqn, vector, flag) NVIC_SetVector((IRQn_Type) (irqn), (uint32_t) (vector))
#define vIRQ_SetVector(irqn, vector)        NVIC_SetVector((IRQn_Type) (irqn), (uint32_t) (vector))
#define vIRQ_GetVector(irqn)                NVIC_GetVector((IRQn_Type) (irqn))
#define vIRQ_EnableIRQ(irqn)                NVIC_EnableIRQ((IRQn_Type) (irqn))
#define vIRQ_DisableIRQ(irqn)               NVIC_DisableIRQ((IRQn_Type) (irqn))
#define vIRQ_ClearPendingIRQ(irqn)          NVIC_ClearPendingIRQ((IRQn_Type) (irqn))
#define vIRQ_SetPendingIRQ(irqn)            NVIC_SetPendingIRQ((IRQn_Type) (irqn))
#define vIRQ_GetPendingIRQ(irqn)            NVIC_GetPendingIRQ((IRQn_Type) (irqn))
#define vIRQ_SetPriority(irqn, priority)    NVIC_SetPriority((IRQn_Type) (irqn), (uint32_t) (priority))
#define vIRQ_GetPriority(irqn)              NVIC_GetPriority((IRQn_Type) (irqn))

/* secure_access.h */
/* the conditional statement will be optimised away since the compiler already
 * knows the sizeof(type) */
#define ADDRESS_READ(type, addr) \
    (sizeof(type) == 4 ? *((volatile uint32_t *) (addr)) : \
     sizeof(type) == 2 ? *((volatile uint16_t *) (addr)) : \
     sizeof(type) == 1 ? *((volatile uint8_t  *) (addr)) : 0)

/* the switch statement will be optimised away since the compiler already knows
 * the sizeof(type) */
#define ADDRESS_WRITE(type, addr, val) \
    { \
        switch(sizeof(type)) \
        { \
            case 4: \
                *((volatile uint32_t *) (addr)) = (uint32_t) (val); \
                break; \
            case 2: \
                *((volatile uint16_t *) (addr)) = (uint16_t) (val); \
                break; \
            case 1: \
                *((volatile uint8_t  *) (addr)) = (uint8_t ) (val); \
                break; \
        } \
    }

#define UNION_READ(type, addr, fieldU, fieldB) ((*((volatile type *) (addr))).fieldB)

/* secure_gateway.h */
#define secure_gateway(dst_box, dst_fn, ...) dst_fn(__VA_ARGS__)

#endif/*__UNSUPPORTED_H__*/
