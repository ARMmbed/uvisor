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

#ifndef __UNVIC_H__
#define __UNVIC_H__

#define UNVIC_MIN_PRIORITY (uint32_t) 1

#define IRQn_OFFSET            16
#define ISR_VECTORS            ((IRQn_OFFSET) + (HW_IRQ_VECTORS))

typedef void (*TIsrVector)(void);

typedef struct {
    TIsrVector hdlr;
    uint8_t    id;
} TIsrUVector;

/* defined in system-specific system.h */
extern const TIsrVector g_isr_vector[ISR_VECTORS];
/* unprivileged interrupts */
extern TIsrUVector g_unvic_vector[HW_IRQ_VECTORS];

extern void     unvic_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag);
extern uint32_t unvic_isr_get(uint32_t irqn);

extern void     unvic_irq_enable(uint32_t irqn);
extern void     unvic_irq_disable(uint32_t irqn);
extern void     unvic_irq_pending_clr(uint32_t irqn);
extern void     unvic_irq_pending_set(uint32_t irqn);
extern uint32_t unvic_irq_pending_get(uint32_t irqn);
extern void     unvic_irq_priority_set(uint32_t irqn, uint32_t priority);
extern uint32_t unvic_irq_priority_get(uint32_t irqn);
extern int      unvic_default(uint32_t isr_id);

extern void unvic_init(void);

void unvic_isr_mux(void);

#endif/*__UNVIC_H__*/
