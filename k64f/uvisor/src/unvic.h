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

#ifndef __UNVIC_H__
#define __UNVIC_H__

typedef struct {
    TIsrVector hdlr;
    uint8_t    id;
} TIsrUVector;

extern TIsrUVector g_unvic_vector[ISR_VECTORS];

extern void unvic_set_isr(uint32_t irqn, uint32_t vector, uint32_t flag);
extern void unvic_let_isr(uint32_t irqn);
extern void unvic_ena_irq(uint32_t irqn);
extern void unvic_dis_irq(uint32_t irqn);
extern void unvic_set_ena_isr(uint32_t irqn, uint32_t vector, uint32_t flag);
extern void unvic_dis_let_isr(uint32_t irqn);
extern void unvic_init(void);

extern uint32_t unvic_get_isr(uint32_t irqn);

#endif/*__UNVIC_H__*/
