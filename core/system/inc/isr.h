#ifndef __ISR_H__
#define __ISR_H__

#define MAX_ISR_VECTORS 52
typedef void (*TIsrVector)(void);
extern TIsrVector g_isr_vector[MAX_ISR_VECTORS];

#endif/*__ISR_H__*/
