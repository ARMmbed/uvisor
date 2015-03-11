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
#ifndef __ISR_H__
#define __ISR_H__

#define IRQn_OFFSET            16
#define ISR_VECTORS            38
#define MAX_ISR_VECTORS        (IRQn_OFFSET + ISR_VECTORS)
#define ISR_SET(irqn, handler) g_isr_vector[IRQn_OFFSET + irqn] = handler;
#define ISR_GET(irqn)          (g_isr_vector[IRQn_OFFSET + irqn])

typedef void (*TIsrVector)(void);
UVISOR_EXTERN TIsrVector g_isr_vector[MAX_ISR_VECTORS];
UVISOR_EXTERN void main_entry(void);
void default_handler(void);

#endif/*__ISR_H__*/
