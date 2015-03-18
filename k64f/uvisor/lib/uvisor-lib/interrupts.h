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

UVISOR_EXTERN void     __uvisor_set_isr(uint32_t irqn, uint32_t vector,
                                        uint32_t flag);
UVISOR_EXTERN uint32_t __uvisor_get_isr(uint32_t irqn);

UVISOR_EXTERN void __uvisor_enable_irq(uint32_t irqn);
UVISOR_EXTERN void __uvisor_disable_irq(uint32_t irqn);

#define uvisor_set_isr(irqn, vector, flag) {		                           \
	if(__uvisor_mode == 0)	                                                   \
	{		                                                                   \
		NVIC_SetVector(irqn, vector);                                          \
	}	                                                                       \
	else	                                                                   \
	{	                                                                       \
		__uvisor_set_isr(irqn, vector, flag);                                  \
	}	                                                                       \
}

#define uvisor_get_isr(irqn) {		                                           \
	if(__uvisor_mode == 0)	                                                   \
	{		                                                                   \
		return NVIC_GetVector(irqn, vector);                                   \
	}	                                                                       \
	else	                                                                   \
	{	                                                                       \
		return __uvisor_get_isr(irqn);                                         \
	}	                                                                       \
}

#define uvisor_enable_irq(irqn) {		                                       \
	if(__uvisor_mode == 0)	                                                   \
	{	                                                                       \
		NVIC_EnableIRQ(irqn);                                                  \
	}	                                                                       \
	else	                                                                   \
	{	                                                                       \
		__uvisor_enable_irq(irqn);                                             \
	}	                                                                       \
}

#define uvisor_disable_irq(irqn) {		                                       \
	if(__uvisor_mode == 0)	                                                   \
	{	                                                                       \
		NVIC_DisableIRQ(irqn);                                                 \
	}	                                                                       \
	else	                                                                   \
	{	                                                                       \
		__uvisor_disable_irq(irqn);                                            \
	}	                                                                       \
}

#endif/*__INTERRUPTS_H__*/
