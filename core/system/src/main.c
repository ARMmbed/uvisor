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
#include "scheduler.h"
#include "svc.h"
#include "unvic.h"
#include "vmpu.h"
#include <stdbool.h>

UVISOR_NOINLINE void uvisor_init_pre(uint32_t const * const user_vtor)
{
    /* Reset the uVisor own BSS section. */
    memset(&__uvisor_bss_start__, 0, VMPU_REGION_SIZE(&__uvisor_bss_start__, &__uvisor_bss_end__));

    /* Initialize the uVisor own data. */
    memcpy(&__uvisor_data_start__, &__uvisor_data_start_src__, VMPU_REGION_SIZE(&__uvisor_data_start__, &__uvisor_data_end__));

    /* Initialize the unprivileged NVIC module. */
    unvic_init(user_vtor);

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

UVISOR_NAKED void main_entry(uint32_t caller)
{
    asm volatile(
        /* Disable all IRQs. They will be re-enabled before de-privileging. */
        "cpsid i\n"
        "isb\n"

        /* Store the bootloader lr value. */
        "push  {r0}\n"

        /* Check for early exit. */
        "bl    main_entry_early_exit\n"
        "cmp   r0, #0\n"
        "it    ne\n"
        "popne {pc}\n"

        /* Set the MSP. Since we are changing stacks we need to pop and re-push
         * the lr value. */
        "pop   {r0}\n"
        "ldr   r1, =__uvisor_stack_top__\n"
        "msr   MSP, r1\n"
        "push  {r0}\n"

        /* First initialization stage. */
        "bl    main_init\n"

        /* Re-enable all IRQs. */
        "cpsie i\n"
        "isb\n"

        /* Pop the lr value. */
        /* Note: This needs to be done now if we then de-privilege exeuction,
         *       otherwise the stack will become unaccessible. */
        "pop   {r0}\n"

#if defined(ARCH_CORE_ARMv7M)
        /* De-privilege execution (ARMv7-M only). */
        "mrs   r1, CONTROL\n"
        "orr   r1, r1, #3\n"
        "msr   CONTROL, r1\n"
#endif /* defined(ARCH_CORE_ARMv7M) */

        /* Return to the caller. */
#if defined(ARCH_CORE_ARMv8M)
        "bic   r0, r0, #1\n"
        "bxns  r0\n"
#else /* defined(ARCH_CORE_ARMv8M) */
        "bx    r0\n"
#endif /* defined(ARCH_CORE_ARMv8M) */
    );
}

bool main_entry_early_exit(void)
{
    return (__uvisor_config.magic != UVISOR_MAGIC || !__uvisor_config.mode || *(__uvisor_config.mode) == 0);
}

void main_init(void)
{
    /* Early uVisor initialization. */
    uvisor_init_pre((uint32_t *) SCB->VTOR);

    /* Run basic sanity checks. */
    /* NOTE: This function halts if there is an error. */
    vmpu_init_pre();

    /* Stack pointers */
    /* Note: The uVisor stack pointer is assumed to be already correctly set. */
    /* Note: We do not need to set the NS NP stack pointer (for the app and the
     *       private boxes), as it is set during the vMPU initialization. */
    uint32_t original_sp = ((uint32_t *) SCB->VTOR)[0] - 4;
#if defined(ARCH_CORE_ARMv8M)
    /* NS P stack pointer, for the RTOS and the uVisor-ns. */
    __TZ_set_MSP_NS(original_sp);

    /* S NP stack pointer, for the SDSs and the transition gateways. */
    __set_PSP((uint32_t) &__uvisor_stack_top_np__);
#else /* defined(ARCH_CORE_ARMv8M) */
    /* NP stack pointer, for the RTOS and the uVisor lib. */
    __set_PSP(original_sp);
#endif /* defined(ARCH_CORE_ARMv8M) */

    /* Set the uVisor vector table */
    SCB->VTOR = (uint32_t) &g_isr_vector;

    /* Finish the uVisor initialization */
    uvisor_init_post();
}

void uvisor_start(void)
{
    scheduler_start();
}
