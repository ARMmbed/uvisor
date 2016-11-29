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
#include "debug.h"
#include "page_allocator.h"
#include "svc.h"
#include "unvic.h"
#include "vmpu.h"

UVISOR_NOINLINE void uvisor_init_pre(void)
{
    /* Reset the uVisor own BSS section. */
    memset(&__uvisor_bss_start__, 0, VMPU_REGION_SIZE(&__uvisor_bss_start__, &__uvisor_bss_end__));

    /* Initialize the uVisor own data. */
    memcpy(&__uvisor_data_start__, &__uvisor_data_start_src__, VMPU_REGION_SIZE(&__uvisor_data_start__, &__uvisor_data_end__));

    /* Initialize the unprivileged NVIC module. */
    unvic_init();

    /* Initialize the debugging features. */
    DEBUG_INIT();
}

static void sanity_check_priv_sys_hooks(UvisorPrivSystemHooks const * priv_sys_hooks)
{
    /* Check that the table is in flash. */
    if (!vmpu_public_flash_addr((uint32_t) priv_sys_hooks) ||
        !vmpu_public_flash_addr((uint32_t) priv_sys_hooks + sizeof(priv_sys_hooks))) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_sys_hooks (0x%08x) not entirely in public flash\n", priv_sys_hooks);
    }

    /*
     * Check that each hook is in flash, if the hook is non-0.
     */
    if (__uvisor_config.priv_sys_hooks->priv_svc_0 &&
        !vmpu_public_flash_addr((uint32_t) __uvisor_config.priv_sys_hooks->priv_svc_0)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_svc_0 (0x%08x) not entirely in public flash\n",
                   __uvisor_config.priv_sys_hooks->priv_svc_0);
    }

    if (__uvisor_config.priv_sys_hooks->priv_pendsv &&
        !vmpu_public_flash_addr((uint32_t) __uvisor_config.priv_sys_hooks->priv_pendsv)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_pendsv (0x%08x) not entirely in public flash\n",
                   __uvisor_config.priv_sys_hooks->priv_pendsv);
    }

    if (__uvisor_config.priv_sys_hooks->priv_systick &&
        !vmpu_public_flash_addr((uint32_t) __uvisor_config.priv_sys_hooks->priv_systick)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_systick (0x%08x) not entirely in public flash\n",
                   __uvisor_config.priv_sys_hooks->priv_pendsv);
    }

    if (__uvisor_config.priv_sys_hooks->priv_os_suspend &&
        !vmpu_public_flash_addr((uint32_t) __uvisor_config.priv_sys_hooks->priv_os_suspend)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_os_suspend (0x%08x) not entirely in public flash\n",
                   __uvisor_config.priv_sys_hooks->priv_os_suspend);
    }

    if (__uvisor_config.priv_sys_hooks->priv_uvisor_semaphore_post &&
        !vmpu_public_flash_addr((uint32_t) __uvisor_config.priv_sys_hooks->priv_uvisor_semaphore_post)) {
        HALT_ERROR(SANITY_CHECK_FAILED, "priv_uvisor_semaphore_post (0x%08x) not entirely in public flash\n",
                   __uvisor_config.priv_sys_hooks->priv_uvisor_semaphore_post);
    }
}

static void load_priv_sys_hooks(void)
{
    /* Make sure the hook table is sane. */
    sanity_check_priv_sys_hooks(__uvisor_config.priv_sys_hooks);

    /*
     * Register each hook.
     */
    if (__uvisor_config.priv_sys_hooks->priv_svc_0) {
        g_priv_sys_hooks.priv_svc_0 = __uvisor_config.priv_sys_hooks->priv_svc_0;
    }

    if (__uvisor_config.priv_sys_hooks->priv_pendsv) {
        g_priv_sys_hooks.priv_pendsv = __uvisor_config.priv_sys_hooks->priv_pendsv;
    }

    if (__uvisor_config.priv_sys_hooks->priv_systick) {
        g_priv_sys_hooks.priv_systick = __uvisor_config.priv_sys_hooks->priv_systick;
    }

    if (__uvisor_config.priv_sys_hooks->priv_os_suspend) {
        g_priv_sys_hooks.priv_os_suspend = __uvisor_config.priv_sys_hooks->priv_os_suspend;
    }

    if (__uvisor_config.priv_sys_hooks->priv_uvisor_semaphore_post) {
        g_priv_sys_hooks.priv_uvisor_semaphore_post = __uvisor_config.priv_sys_hooks->priv_uvisor_semaphore_post;
    }
}

UVISOR_NOINLINE void uvisor_init_post(void)
{
    /* Inititalize the MPU. */
    vmpu_init_post();

    /* Initialize the page allocator. */
    page_allocator_init(__uvisor_config.page_start, __uvisor_config.page_end, __uvisor_config.page_size);

    /* Initialize the SVCall interface. */
    svc_init();

    /* Load the privileged system hooks. */
    load_priv_sys_hooks();

    DPRINTF("uvisor initialized\n");
}

UVISOR_NOINLINE
void main_init(uint32_t caller);

void main_entry(uint32_t caller)
{
    /* Force the compiler to store the caller in r0, not on the stack! */
    register uint32_t r0 asm("r0") = caller;

    /* Return immediately if the magic is invalid or uVisor is disabled.
     * This ensures that no uVisor feature that could halt the system is
     * active in disabled mode (for example, printing debug messages to the
     * semihosting port). */
    if (__uvisor_config.magic != UVISOR_MAGIC || !__uvisor_config.mode || *(__uvisor_config.mode) == 0) {
        return;
    }

    __set_MSP((uint32_t) &__uvisor_stack_top__);
    /* We've switched stacks, r0 still contains the caller though! */
    main_init(r0);

    /* FIXME: Function return does not clean up the stack! */
    __builtin_unreachable();
}

void main_init(uint32_t caller)
{
    /* Early uVisor initialization. */
    uvisor_init_pre();

    /* Run basic sanity checks. */
    if (vmpu_init_pre()) {
        /* If uVisor is disabled, return immediately. */
        asm volatile("bx  %[caller]\n" :: [caller] "r" (caller));
        /* FIXME: Function return does not clean up the stack! */
        __builtin_unreachable();
    }

    /* Disable all IRQs to perform atomic pointers swaps. */
    __disable_irq();

    uint32_t msp = ((uint32_t *) SCB->VTOR)[0];
#if defined(ARCH_MPU_ARMv8M)
    /* Set the S and NS vector tables. */
    SCB_NS->VTOR = SCB->VTOR;
    __TZ_set_PSP_NS(msp);
    msp -= 0x200; /* XXX Reserve 128 bytes for the NS PSP stack from the MSP NS
    stack. RTOS will use thread stacks for PSP soon. NS PSP must be valid for
    RTOS SVC to work. We need RTOS SVC to work in order to call
    osSemaphoreWait. We need to call osSemaphoreWait in order to initialize a
    semaphore as having no availability. We need to initialize semaphores as
    part of box initialization. We initialize the boxes before the scheduler
    runs to avoid conflicts between the scheduler and uVisor. We don't want the
    RTOS to change threads or boxes while we are in the middle of initializing
    the boxes. */
    __TZ_set_MSP_NS(msp);

    __TZ_set_CONTROL_NS(__TZ_get_CONTROL_NS() | 2);

    __set_PSP((uint32_t)&__uvisor_stack_top_np__);
#else
    __set_PSP(msp);
#endif /* defined(ARCH_MPU_ARMv8M) */
    SCB->VTOR = (uint32_t) &g_isr_vector;

    /* Re-enable all IRQs. */
    __enable_irq();

    /* Finish the uVisor initialization */
    uvisor_init_post();

#if defined(ARCH_MPU_ARMv8M)
    /* Branch to the caller. This switches to NS mode, and is executed in
     * privileged mode. */
    caller &= ~(0x1UL);
    asm volatile("bxns %[caller]\n" :: [caller] "r" (caller));
#else /* defined(ARCH_MPU_ARMv8M) */

    /* Save caller into a register so we can still access it after we
     * deprivilege. */
    register uint32_t r0 asm("r0") = caller | 1;

    /* Switch to unprivileged mode.
     * This is possible as the uVisor code is readable in unprivileged mode. */
    __set_CONTROL(__get_CONTROL() | 3);
    __ISB();
    __DSB();
    /* Branch to the caller. This will be executed in unprivileged mode. */
    asm volatile("bx  %[r0]\n" :: [r0] "r" (r0));
#endif /* defined(ARCH_MPU_ARMv8M) */
    /* FIXME: Function return does not clean up the stack! */
    __builtin_unreachable();
}
