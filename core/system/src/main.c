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
#if defined(ARCH_CORE_ARMv7M)
#include "priv_sys_hooks.h"
#endif /* defined(ARCH_CORE_ARMv7M) */
#include "scheduler.h"
#include "svc.h"
#include "virq.h"
#include "vmpu.h"
#include <stdbool.h>

UVISOR_NOINLINE void uvisor_init_pre(uint32_t const * const user_vtor)
{
    /* Reset the uVisor own BSS section. */
    memset(&__uvisor_bss_start__, 0, VMPU_REGION_SIZE(&__uvisor_bss_start__, &__uvisor_bss_end__));

    /* Initialize the uVisor own data. */
    memcpy(&__uvisor_data_start__, &__uvisor_data_start_src__, VMPU_REGION_SIZE(&__uvisor_data_start__, &__uvisor_data_end__));

    /* Initialize the unprivileged NVIC module. */
    virq_init(user_vtor);

    /* Initialize the debugging features. */
    DEBUG_INIT();
}

UVISOR_NOINLINE void uvisor_init_post(void)
{
    /* Inititalize the MPU. */
    vmpu_init_post();

    /* Initialize the page allocator. */
    page_allocator_init(__uvisor_config.page_start, __uvisor_config.page_end, __uvisor_config.page_size);

#if defined(ARCH_CORE_ARMv7M)
    /* Initialize the SVCall interface. */
    svc_init();

    /* Load the privileged system hooks. */
    priv_sys_hooks_load();
#endif /* defined(ARCH_CORE_ARMv7M) */

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

        /* Check if uVisor config is sane and enabled. */
        "bl    uvisor_config_is_sane_and_enabled\n"
        "cmp   r0, #0\n"
        "it    eq\n"
        "popeq {pc}\n"

        /* Set the MSP and MSPLIM. Since we are changing stacks we need to pop and re-push
         * the lr value. */
        "pop   {r0}\n"
        "ldr   r1, =__uvisor_stack_top__\n"
        "msr   MSP, r1\n"
#if defined(ARCH_CORE_ARMv8M)
        "ldr   r1, =__uvisor_stack_start__\n"
        "msr   MSPLIM, r1\n"
#endif 
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

        /* ISB to ensure subsequent instructions are fetched with the correct privilege level */
        "isb\n"
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

/* Return true if the uvisor_config magic matches. */
static bool uvisor_config_magic_match(void)
{
    return __uvisor_config.magic == UVISOR_MAGIC;
}

/* Return true if uVisor is enabled. */
static bool uvisor_config_enabled(void)
{
    return __uvisor_config.mode && *(__uvisor_config.mode) != 0;
}

/* Halt if uVisor magic doesn't match. Return true if uVisor is enabled, false
 * if disabled. */
bool uvisor_config_is_sane_and_enabled(void)
{
    if (!uvisor_config_magic_match()) {
        /* uVisor magic didn't match. Halt. */
        HALT_ERROR(SANITY_CHECK_FAILED, "Bad uVisor config magic");
    }

    return uvisor_config_enabled();
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

    /* NS P limit, for the RTOS and the uVisor-ns. */
    __TZ_set_MSPLIM_NS(__uvisor_config.public_box_stack_limit);

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
#if defined(ARCH_CORE_ARMv8M)
    scheduler_start();
#endif /* defined(ARCH_CORE_ARMv8M) */
}
