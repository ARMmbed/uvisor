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
#include <mbed/mbed.h>
#include <uvisor-lib/uvisor-lib.h>

void __uvisor_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    register uint32_t __r1 __asm__("r1") = vector;
    register uint32_t __r2 __asm__("r2") = flag;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [r1]     "r" (__r1),
          [r2]     "r" (__r2),
          [svc_id] "i" (UVISOR_SVC_ID_ISR_SET)
    );
}

uint32_t __uvisor_isr_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_ISR_GET)
    );
    return __res;
}

void __uvisor_irq_enable(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_ENABLE)
    );
}

void __uvisor_irq_disable(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_DISABLE)
    );
}

void __uvisor_irq_pending_clr(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PEND_CLR)
    );
}

void __uvisor_irq_pending_set(uint32_t irqn)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PEND_SET)
    );
}

uint32_t __uvisor_irq_pending_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_IRQ_PEND_GET)
    );
    return __res;
}

void __uvisor_irq_priority_set(uint32_t irqn, uint32_t priority)
{
    register uint32_t __r0 __asm__("r0") = irqn;
    register uint32_t __r1 __asm__("r1") = priority;
    __asm__ volatile(
        "svc %[svc_id]\n"
        :
        : [r0]     "r" (__r0),
          [r1]     "r" (__r1),
          [svc_id] "i" (UVISOR_SVC_ID_IRQ_PRIO_SET)
    );
}

uint32_t __uvisor_irq_priority_get(uint32_t irqn)
{
    register uint32_t __r0  __asm__("r0") = irqn;
    register uint32_t __res __asm__("r0");
    __asm__ volatile(
        "svc %[svc_id]\n"
        : [res]    "=r" (__res)
        : [r0]     "r"  (__r0),
          [svc_id] "i"  (UVISOR_SVC_ID_IRQ_PRIO_GET)
    );
    return __res;
}
