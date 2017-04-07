/*
 * Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
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
#include "halt.h"
#include "vmpu.h"
#include <stdint.h>

static void priv_sys_hooks_sanity_check(UvisorPrivSystemHooks const * priv_sys_hooks)
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

void priv_sys_hooks_load(void)
{
    /* Make sure the hook table is sane. */
    priv_sys_hooks_sanity_check(__uvisor_config.priv_sys_hooks);

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

/* Privileged PendSV and SysTick hooks assume that they are called directly by
 * hardware. So, these handlers need their stack frame and registers to look
 * exactly like they were called directly by hardware. This means LR needs to
 * be EXC_RETURN, not some value placed there by the C compiler when it does a
 * branch with link. */

void UVISOR_NAKED PendSV_IRQn_Handler(void)
{
    asm volatile(
        "ldr  r0, %[priv_pendsv]\n"  /* Load the hook from the hook table. */
        "bx   r0\n"                  /* Branch to the hook (without link). */
        :: [priv_pendsv] "m" (g_priv_sys_hooks.priv_pendsv)
    );
}

void UVISOR_NAKED SysTick_IRQn_Handler(void)
{
    asm volatile(
        "ldr  r0, %[priv_systick]\n" /* Load the hook from the hook table. */
        "bx   r0\n"                  /* Branch to the hook (without link). */
        :: [priv_systick] "m" (g_priv_sys_hooks.priv_systick)
    );
}

/* Default privileged system hooks (placed in SRAM) */
UvisorPrivSystemHooks g_priv_sys_hooks = {
    .priv_svc_0 = isr_default_sys_handler,
    .priv_pendsv = isr_default_sys_handler,
    .priv_systick = isr_default_sys_handler,
};
