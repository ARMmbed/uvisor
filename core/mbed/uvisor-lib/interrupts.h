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
#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

UVISOR_EXTERN void     __uvisor_isr_set(uint32_t irqn, uint32_t vector,
                                        uint32_t flag);
UVISOR_EXTERN uint32_t __uvisor_isr_get(uint32_t irqn);

UVISOR_EXTERN void     __uvisor_irq_enable(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_disable(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_pending_clr(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_pending_set(uint32_t irqn);
UVISOR_EXTERN uint32_t __uvisor_irq_pending_get(uint32_t irqn);
UVISOR_EXTERN void     __uvisor_irq_priority_set(uint32_t irqn,
                                                 uint32_t priority);
UVISOR_EXTERN uint32_t __uvisor_irq_priority_get(uint32_t irqn);

#define vIRQ_SetVector(irqn, vector, flag)                                     \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetVector((IRQn_Type) (irqn), (uint32_t) (vector));           \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_isr_set((uint32_t) (irqn), (uint32_t) (vector),           \
                             (uint32_t) (flag));                               \
        }                                                                      \
    })

#define vIRQ_GetVector(irqn)                                                   \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetVector((IRQn_Type) (irqn));                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res = __uvisor_isr_get((uint32_t) (irqn));                         \
        }                                                                      \
        res;                                                                   \
    })

#define vIRQ_EnableIRQ(irqn)                                                   \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_EnableIRQ((IRQn_Type) (irqn));                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_enable((uint32_t) (irqn));                            \
        }                                                                      \
    })

#define vIRQ_DisableIRQ(irqn)                                                  \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_DisableIRQ((IRQn_Type) (irqn));                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_disable((uint32_t) (irqn));                           \
        }                                                                      \
    })

#define vIRQ_ClearPendingIRQ(irqn)                                             \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_ClearPendingIRQ((IRQn_Type) (irqn));                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_pending_clr((uint32_t) (irqn));                       \
        }                                                                      \
    })

#define vIRQ_SetPendingIRQ(irqn)                                               \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetPendingIRQ((IRQn_Type) (irqn));                            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_pending_set((uint32_t) (irqn));                       \
        }                                                                      \
    })

#define vIRQ_GetPendingIRQ(irqn)                                               \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetPendingIRQ((IRQn_Type) (irqn));                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res =  __uvisor_irq_pending_get((uint32_t) (irqn));                \
        }                                                                      \
        res;                                                                   \
    })

#define vIRQ_SetPriority(irqn, priority)                                       \
    ({                                                                         \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            NVIC_SetPriority((IRQn_Type) (irqn), (uint32_t) (priority));       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            __uvisor_irq_priority_set((uint32_t) (irqn),                       \
                                      (uint32_t) (priority));                  \
        }                                                                      \
    })

#define vIRQ_GetPriority(irqn)                                                 \
    ({                                                                         \
        uint32_t res;                                                          \
        if(__uvisor_mode == 0)                                                 \
        {                                                                      \
            res = NVIC_GetPriority((IRQn_Type) (irqn));                        \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res =  __uvisor_irq_priority_get((uint32_t) (irqn));               \
        }                                                                      \
        res;                                                                   \
    })

#endif/*__INTERRUPTS_H__*/
