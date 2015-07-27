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
#include <uvisor.h>
#include "halt.h"
#include "unvic.h"
#include "svc.h"
#include "debug.h"

/* unprivileged vector table */
TIsrUVector g_unvic_vector[HW_IRQ_VECTORS];

/* vmpu_acl_irq to unvic_acl_add */
void vmpu_acl_irq(uint8_t box_id, void *function, uint32_t irqn)
     UVISOR_LINKTO(unvic_acl_irq);

static void unvic_check_acl(int irqn)
{
    /* don't allow to modify uVisor-owned IRQs */
    if(!unvic_default(irqn))
        HALT_ERROR(NOT_ALLOWED,
                   "access denied: IRQ %d is not multiplexed\n\r", irqn);

    /* check if the same box that registered the ISR is modifying it */
    if(svc_cx_get_curr_id() != g_unvic_vector[irqn].id)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is owned by box %d\n\r", irqn,
                                               g_unvic_vector[irqn].id);
    }
}

/* FIXME check if allowed (ACL) */
/* FIXME flag is currently not implemented */
void unvic_isr_set(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    uint32_t curr_id;

    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* get current ID to check ownership of the IRQn slot */
    curr_id = svc_cx_get_curr_id();

    /* save unprivileged handler (unvic_default_handler if 0) */
    g_unvic_vector[irqn].hdlr = (TIsrVector) vector;
    g_unvic_vector[irqn].id   = vector ? curr_id : 0;

    /* set default priority (SVC must always be higher) */
    NVIC_SetPriority(irqn, UNVIC_MIN_PRIORITY);

    DPRINTF("IRQ %d %s box %d\n\r",
            irqn,
            vector ? "registered to" : "released by",
            svc_cx_get_curr_id());
}

uint32_t unvic_isr_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    return (uint32_t)g_unvic_vector[irqn].hdlr;
}

/* FIXME check if allowed (ACL) */
void unvic_irq_enable(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* enable IRQ */
    DPRINTF("IRQ %d enabled\n\r", irqn);
    NVIC_EnableIRQ(irqn);
}

void unvic_irq_disable(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    DPRINTF("IRQ %d disabled, but still owned by box %d\n\r", irqn,
                                          g_unvic_vector[irqn].id);
    NVIC_DisableIRQ(irqn);
}

/* FIXME check if allowed (ACL) */
void unvic_irq_pending_clr(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* enable IRQ */
    DPRINTF("IRQ %d pending status cleared\n\r", irqn);
    NVIC_ClearPendingIRQ(irqn);
}

/* FIXME check if allowed (ACL) */
void unvic_irq_pending_set(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* enable IRQ */
    DPRINTF("IRQ %d pending status set "
            "(will be served as soon as possible)\n\r", irqn);
    NVIC_SetPendingIRQ(irqn);
}

uint32_t unvic_irq_pending_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* get priority for device specific interrupts  */
    return NVIC_GetPendingIRQ(irqn);
}

void unvic_irq_priority_set(uint32_t irqn, uint32_t priority)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* check priority */
    if(priority < UNVIC_MIN_PRIORITY)
        priority = UNVIC_MIN_PRIORITY;

    /* set priority for device specific interrupts */
    NVIC_SetPriority(irqn, priority);
}

uint32_t unvic_irq_priority_get(uint32_t irqn)
{
    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    /* get priority for device specific interrupts  */
    return NVIC_GetPriority(irqn);
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

    /* verify IRQ access privileges */
    unvic_check_acl(irqn);

    dst_id = g_unvic_vector[irqn].id;
    dst_fn = (uint32_t) g_unvic_vector[irqn].hdlr;

    /* check ISR vector */
    if(!dst_fn)
    {
        HALT_ERROR(NOT_ALLOWED,
                   "Unprivileged handler for IRQ %i not found\n\r", irqn);
    }

    /* gather information from current state */
    src_id = svc_cx_get_curr_id();
    src_sp = svc_cx_validate_sf((uint32_t *) __get_PSP());

    /* a proper context switch is only needed if changing box */
    if(src_id != dst_id)
    {
        /* gather information from current state */
        dst_sp = svc_cx_get_curr_sp(dst_id);

        /* set the context stack pointer for the dst box */
        __uvisor_box_context = g_svc_cx_context_ptr[dst_id];

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
        /* set the context stack pointer back to the one of the src box */
        __uvisor_box_context = g_svc_cx_context_ptr[src_id];

        /* switch ACls */
        vmpu_switch(dst_id, src_id);
    }

    /* re-privilege execution */
    __set_PSP((uint32_t) src_sp);
    __set_CONTROL(__get_CONTROL() & ~2);
}

void unvic_acl_irq(uint8_t box_id, void *function, uint32_t isr_id)
{
    TIsrUVector *uv;

    /* don't allow to modify uVisor-owned IRQs */
    if(!unvic_default(isr_id))
        HALT_ERROR(NOT_ALLOWED,
                   "access denied: IRQ %d is not multiplexed\n\r", isr_id);

    /* get vector entry */
    uv = &g_unvic_vector[isr_id];

    /* check if IRQ entry is populated */
    if(uv->id || uv->hdlr)
    {
        HALT_ERROR(PERMISSION_DENIED,
                   "access denied: IRQ %d is owned by box %d\n\r", isr_id,
                                               uv->id);
    }

    /* save settings, function handler is optional */
    uv->id = box_id;
    uv->hdlr = function;
}

int unvic_default(uint32_t isr_id)
{
    if( isr_id>=(HW_IRQ_VECTORS) )
        return 0;

    return g_isr_vector[isr_id+IRQn_OFFSET] == isr_default_handler;
}

void unvic_init(void)
{
}
