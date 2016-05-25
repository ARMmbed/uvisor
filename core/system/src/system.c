/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <uvisor.h>

/* All system IRQs are by default weakly linked to the system default handler */
void UVISOR_ALIAS(isr_default_sys_handler) NonMaskableInt_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) HardFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) MemoryManagement_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) BusFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) UsageFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) SVCall_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) DebugMonitor_IRQn_Handler(void);

/* Privileged PendSV and SysTick handlers assume that they are called as the by
 * the hardware. So, these handlers need their stack frame to look exactly like
 * they were called directly by hardware. This means LR needs to be EXC_RETURN,
 * not some value placed there by the C compiler when it does a branch with
 * link. */
void UVISOR_NAKED PendSV_IRQn_Handler(void)
{
    asm volatile(
        "ldr  r0, =__uvisor_config\n"
        "ldr  r0, [r0, %[priv_pendsv]]\n"
        "cmp  r0, #0\n"                  // Was a handler registered?
        "it   ne\n"
        "bxne r0\n"                      // Yes, jump to it.
        "b    isr_default_sys_handler\n" // No, run the default handler.
        :: [priv_pendsv] "i" (offsetof(UvisorConfig, priv_pendsv))
    );
}

void UVISOR_NAKED SysTick_IRQn_Handler(void)
{
    asm volatile(
        "ldr  r0, =__uvisor_config\n"
        "ldr  r0, [r0, %[priv_systick]]\n"
        "cmp  r0, #0\n"                  // Was a handler registered?
        "it   ne\n"
        "bxne r0\n"                      // Yes, jump to it.
        "b    isr_default_sys_handler\n" // No, run the default handler.
        :: [priv_systick] "i" (offsetof(UvisorConfig, priv_systick))
    );
}

/* Default vector table (placed in Flash) */
__attribute__((section(".isr"))) const TIsrVector g_isr_vector[ISR_VECTORS] =
{
    /* Initial stack pointer */
    (TIsrVector) &__stack_top__,

    /* System IRQs */
    &main_entry,                           /* -15 */
    NonMaskableInt_IRQn_Handler,           /* -14 */
    HardFault_IRQn_Handler,                /* -13 */
    MemoryManagement_IRQn_Handler,         /* -12 */
    BusFault_IRQn_Handler,                 /* -11 */
    UsageFault_IRQn_Handler,               /* -10 */
    isr_default_sys_handler,               /* - 9 */
    isr_default_sys_handler,               /* - 8 */
    isr_default_sys_handler,               /* - 7 */
    isr_default_sys_handler,               /* - 6 */
    SVCall_IRQn_Handler,                   /* - 5 */
    DebugMonitor_IRQn_Handler,             /* - 4 */
    isr_default_sys_handler,               /* - 3 */
    PendSV_IRQn_Handler,                   /* - 2 */
    SysTick_IRQn_Handler,                  /* - 1 */

    /* NVIC IRQs */
    /* Note: This is a GCC extension. */
    [NVIC_OFFSET ... (ISR_VECTORS - 1)] = isr_default_handler
};

void UVISOR_NAKED UVISOR_NORETURN isr_default_sys_handler(void)
{
    /* Handle system IRQ with an unprivileged handler. */
    /* Note: The original lr and the MSP are passed to the actual handler */
    asm volatile(
        "mov r0, lr\n"
        "mrs r1, MSP\n"
        "push {lr}\n"
        "blx vmpu_sys_mux_handler\n"
        "pop {pc}\n"
    );
}

void UVISOR_NAKED UVISOR_NORETURN isr_default_handler(void)
{
    /* Handle NVIC IRQ with an unprivileged handler.
     * Serving an IRQn in unprivileged mode is achieved by mean of two SVCalls:
     * The first one de-previliges execution, the second one re-privileges it. */
    /* Note: NONBASETHRDENA (in SCB) must be set to 1 for this to work. */
    asm volatile(
        "svc  %[unvic_in]\n"
        "svc  %[unvic_out]\n"
        "bx   lr\n"
        ::[unvic_in]  "i" ((UVISOR_SVC_ID_UNVIC_IN) & 0xFF),
          [unvic_out] "i" ((UVISOR_SVC_ID_UNVIC_OUT) & 0xFF)
    );
}
