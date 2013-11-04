#ifndef __ISR_H__
#define __ISR_H__

#define MAX_ISR_VECTORS 52
#define IRQn_OFFSET 16
#define ISR_SET(irqn, handler) g_isr_vector[IRQn_OFFSET + SVCall_IRQn] = handler;

typedef void (*TIsrVector)(void);
extern TIsrVector g_isr_vector[MAX_ISR_VECTORS];

#endif/*__ISR_H__*/
