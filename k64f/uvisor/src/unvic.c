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
#include <uvisor.h>
#include <isr.h>
#include "halt.h"
#include "unvic.h"
#include "svc.h"
#include "debug.h"

/* unprivileged vector table */
TIsrUVector g_unvic_vector[IRQ_VECTORS];

/* isr_default_handler to unvic_default_handler */
void isr_default_handler(void) UVISOR_LINKTO(unvic_default_handler);

/* unprivileged default handler */
void unvic_default_handler(void)
{
    HALT_ERROR(NOT_ALLOWED, "spurious IRQ (IPSR=%i)", (uint8_t)__get_IPSR());
}

/* FIXME check if allowed (ACL) */
/* FIXME flag is currently not implemented */
void unvic_set_isr(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    uint32_t curr_id;

    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* get current ID to check ownership of the IRQn slot */
    curr_id = svc_cx_get_curr_id();

    /* check if the same box that registered the ISR is modifying it */
    if(ISR_GET(irqn) != (TIsrVector) &unvic_default_handler &&
       svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    /* save unprivileged handler (unvic_default_handler if 0) */
    g_unvic_vector[irqn].hdlr = vector ? (TIsrVector) vector :
                                         &unvic_default_handler;
    g_unvic_vector[irqn].id   = curr_id;

    /* set privileged handler to unprivleged mux */
    ISR_SET(irqn, &unvic_isr_mux);

    /* set default priority (SVC must always be higher) */
    NVIC_SetPriority(irqn, UNVIC_MIN_PRIORITY);

    DPRINTF("IRQ %d is now registered to box %d with vector 0x%08X\n\r",
             irqn, svc_cx_get_curr_id(), vector);
}

uint32_t unvic_get_isr(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* if the vector is unregistered return 0, otherwise the vector */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
        return 0;
    else
        return (uint32_t) g_unvic_vector[irqn].hdlr;
}

/* FIXME check if allowed (ACL) */
void unvic_enable_irq(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* check if ISR has been set for this IRQn slot */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "no ISR set yet for IRQ %d\n\r", irqn);
    }

    /* check if the same box that registered the ISR is enabling it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    /* enable IRQ */
    DPRINTF("IRQ %d is now active\n\r", irqn);
    NVIC_EnableIRQ(irqn);
}

void unvic_disable_irq(uint32_t irqn)
{
    /* check IRQn */
    if(irqn >= IRQ_VECTORS)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "IRQ %d out of range (%d to %d)\n\r", irqn, 0, IRQ_VECTORS);
    }

    /* check if ISR has been set for this IRQn slot */
    if(ISR_GET(irqn) == (TIsrVector) &unvic_default_handler)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "no ISR set yet for IRQ %d\n\r", irqn);
    }

    /* check if the same box that registered the ISR is disabling it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }

    DPRINTF("IRQ %d is now disabled, but still owned by box %d\n\r", irqn,
                                                 g_unvic_vector[irqn].id);
    NVIC_DisableIRQ(irqn);
}

void unvic_set_priority(uint32_t irqn, uint32_t priority)
{
    /* unprivileged code cannot set priorities for system interrupts */
    if((int32_t) irqn < 0)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is a system interrupt and is owned\
                    by uVisor\n\r", irqn);
    }
    else
    {
        /* check IRQn */
        if(irqn >= IRQ_VECTORS)
        {
            HALT_ERROR(NOT_ALLOWED,
                       "IRQ %d out of range (%d to %d)\n\r",
                       irqn, 0, IRQ_VECTORS);
        }

        /* check priority */
        if(priority < UNVIC_MIN_PRIORITY)
        {
            HALT_ERROR(PERMISSION_DENIED,
                       "access denied: mimimum allowed priority is %d\n\r",
                       UNVIC_MIN_PRIORITY);
        }

        /* set priority for device specific interrupts */
        NVIC->IP[irqn] = ((priority << (8 - __NVIC_PRIO_BITS)) & 0xff);
    }

}

uint32_t unvic_get_priority(uint32_t irqn)
{
    /* unprivileged code only see a default 0-priority for system interrupts */
    if((int32_t) irqn < 0)
    {
        return 0;
    }
    else
    {
        /* check IRQn */
        if(irqn >= IRQ_VECTORS)
        {
            HALT_ERROR(NOT_ALLOWED,
                       "IRQ %d out of range (%d to %d)\n\r",
                       irqn, 0, IRQ_VECTORS);
        }

        /* get priority for device specific interrupts  */
        return (uint32_t) (NVIC->IP[irqn] >> (8 - __NVIC_PRIO_BITS));
    }
}

/* ISR multiplexer to deprivilege execution of an interrupt routine */
void unvic_isr_mux(void)
{
    /* ISR number */
    uint32_t irqn = (uint32_t)(uint8_t) __get_IPSR() - IRQn_OFFSET;
    if(!NVIC_GetActive(irqn))
    {
        HALT_ERROR(NOT_ALLOWED,
                   "unvic_isr_mux() executed with no active IRQ\n\r");
    }

    /* ISR vector */
    if(!g_unvic_vector[irqn].hdlr)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "unprivileged handler for IRQ %d not found\n\r", irqn);
    }

    /* handle IRQ with an unprivileged handler */
    DPRINTF("IRQ %d being served with vector 0x%08X\n\r", irqn,
                                                          g_unvic_vector[irqn]);
    asm volatile(
        "svc  %[unvic_in]\n"
        "svc  %[unvic_out]\n"
        ::[unvic_in]  "i" (UVISOR_SVC_ID_UNVIC_IN),
          [unvic_out] "i" (UVISOR_SVC_ID_UNVIC_OUT)
    );
}

/* naked wrapper function needed to preserve LR */
void UVISOR_NAKED unvic_svc_cx_in(uint32_t *svc_sp, uint32_t svc_pc)
{
    asm volatile(
        "push {r4 - r11}\n"         /* store callee-saved regs on MSP (priv) */
        "push {lr}\n"               /* save lr for later */
        // r0 = svc_sp              /* 1st arg: SVC sp */
        // r1 = svc_pc              /* 1st arg: SVC pc */
        "bl   __unvic_svc_cx_in\n"  /* execute handler and return */
        "cmp  r0, #0\n"             /* check return flag */
        "beq  no_regs_clearing\n"
        "mov  r4,  #0\n"            /* clear callee-saved regs on request */
        "mov  r5,  #0\n"
        "mov  r6,  #0\n"
        "mov  r7,  #0\n"
        "mov  r8,  #0\n"
        "mov  r9,  #0\n"
        "mov  r10, #0\n"
        "mov  r11, #0\n"
        "no_regs_clearing:\n"
        "pop  {lr}\n"               /* restore lr */
        "orr  lr, #0x1C\n"          /* return with PSP, 8 words */
        "bx   lr\n"
        /* the unprivileged interrupt handler will be executed after this */
        :: "r" (svc_sp), "r" (svc_pc)
    );
}

uint32_t __unvic_svc_cx_in(uint32_t *svc_sp, uint32_t svc_pc)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;
    uint32_t           dst_fn;
    uint32_t           dst_sp_align;

    /* this handler is always executed from privileged code */
    uint32_t *msp = svc_sp;

    /* gather information from IRQn */
    uint32_t ipsr = msp[7];
    uint32_t irqn = (ipsr & 0x1FF) - IRQn_OFFSET;
    dst_id = g_unvic_vector[irqn].id;
    dst_fn = (uint32_t) g_unvic_vector[irqn].hdlr;

    /* gather information from current state */
    src_id = svc_cx_get_curr_id();
    src_sp = svc_cx_validate_sf((uint32_t *) __get_PSP());

    /* a proper context switch is only needed if changing box */
    if(src_id != dst_id)
    {
        /* gather information from current state */
        dst_sp = svc_cx_get_curr_sp(dst_id);

        /* switch boxes */
        vmpu_switch(src_id, dst_id);
    }
    else
    {
        dst_sp = src_sp;
    }

    /* create unprivileged stack frame */
    dst_sp_align = ((uint32_t) dst_sp & 0x4) ? 1 : 0;
    dst_sp      -= (SVC_CX_EXC_SF_SIZE - dst_sp_align);

    /* create stack frame for de-privileging the interrupt:
     *   - all general purpose registers are ignored
     *   - lr is the next SVCall
     *   - the return address is the unprivileged handler
     *   - xPSR is copied from the MSP
     *   - pending IRQs are cleared in iPSR
     *   - store alignment for future unstacking */
    dst_sp[5] = ((uint32_t) svc_pc + 2) | 1;             /* lr */
    dst_sp[6] = dst_fn;                                  /* return address */
    dst_sp[7] = ipsr;                                    /* xPSR */
    dst_sp[7] &= ~0x1FF;                                 /* IPSR - clear IRQn */
    dst_sp[7] |= dst_sp_align << 9;                      /* xPSR - alignment */

    /* save the current state */
    svc_cx_push_state(src_id, src_sp, dst_id);
    DEBUG_CX_SWITCH_IN();

    /* de-privilege executionn */
    __set_PSP((uint32_t) dst_sp);
    __set_CONTROL(__get_CONTROL() | 3);

    /* FIXME add support for privacy (triggers register clearing) */
    return 0;
}

/* naked wrapper function needed to preserve MSP and LR */
void UVISOR_NAKED unvic_svc_cx_out(uint32_t *svc_sp)
{
    asm volatile(
        // r0 = svc_sp                  /* 1st arg: SVC sp */
        "mrs r1, MSP\n"                 /* 2nd arg: msp */
        "add r1, #32\n"                 /* account for callee-saved registers */
        "push {lr}\n"                   /* save lr for later */
        "bl __unvic_svc_cx_out\n"       /* execute handler and return */
        "pop  {lr}\n"                   /* restore lr */
        "pop  {r4-r11}\n"               /* restore callee-saved registers */
        "orr lr, #0x10\n"               /* return with MSP, 8 words */
        "bic lr, #0xC\n"
        "bx  lr\n"
        :: "r" (svc_sp)
    );
}

void __unvic_svc_cx_out(uint32_t *svc_sp, uint32_t *msp)
{
    uint8_t   src_id,  dst_id;
    uint32_t *src_sp, *dst_sp;

    /* gather information from current state */
    dst_id = svc_cx_get_curr_id();
    dst_sp = svc_cx_validate_sf(svc_sp);

    /* gather information from previous state */
    svc_cx_pop_state(dst_id, dst_sp);
    src_id = svc_cx_get_src_id();
    src_sp = svc_cx_get_src_sp();
    DEBUG_CX_SWITCH_OUT();

    /* copy return address of previous stack frame to the privileged one, which
     * was kept idle after interrupt de-privileging */
    msp[6] = dst_sp[6];

    if(src_id != dst_id)
    {
        /* switch ACls */
        vmpu_switch(dst_id, src_id);
    }

    /* re-privilege execution */
    __set_PSP((uint32_t) src_sp);
    __set_CONTROL(__get_CONTROL() & ~2);
}

void unvic_init(void)
{
    /* call original ISR initialization */
    isr_init();
}
