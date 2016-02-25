/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#ifndef __HARDWARE_SUPPORT_H__
#define __HARDWARE_SUPPORT_H__

/* Architecture specifications */
#define CORE_IMPLEMENTER  0x41 /* Implementer  == ARM */
#define CORE_ARCHITECTURE 0xF  /* Architecture == ARMv7-M */

/* Core-specific architecture specifications */
#if defined(CORE_CORTEX_M3)

/* Note: Currently the revision requirement (>= r2p0) is needed because we use
 *       the SCnSCB->ACTLR register to disable write buffering in debug mode. */
#define CORE_PARTNO        0xC23 /* PartNO   == Cortex-M3 */
#define CORE_REVISTION_MIN 0x2   /* Revision >= r2 */
#define CORE_VARIANT_MIN   0x0   /* Variant  >= p0 */

#elif defined(CORE_CORTEX_M4)

#define CORE_PARTNO        0xC24 /* PartNO   == Cortex-M4 */
#define CORE_REVISTION_MIN 0x0   /* Revision >= r0 */
#define CORE_VARIANT_MIN   0x0   /* Variant  >= p0 */

#else /* defined(CORE_CORTEX_M3) || defined(CORE_CORTEX_M4) */

#error "Unsupported ARM core. Make sure CORE_* is defined in your workspace."

#endif /* Unsupported ARM core */

/* These macros will be used to check the core specifications at runtime.
 * They return true if the check succeeds. */
#define CORE_VERSION_CHECK() \
    ((((SCB->CPUID & SCB_CPUID_IMPLEMENTER_Msk) >> SCB_CPUID_IMPLEMENTER_Pos) == CORE_IMPLEMENTER) && \
     (((SCB->CPUID & SCB_CPUID_ARCHITECTURE_Msk) >> SCB_CPUID_ARCHITECTURE_Pos) == CORE_ARCHITECTURE) && \
     (((SCB->CPUID & SCB_CPUID_PARTNO_Msk) >> SCB_CPUID_PARTNO_Pos) == CORE_PARTNO))
#define CORE_REVISION_CHECK() \
    ((((SCB->CPUID & SCB_CPUID_VARIANT_Msk) >> SCB_CPUID_VARIANT_Pos) >= CORE_VARIANT_MIN) && \
     (((SCB->CPUID & SCB_CPUID_REVISION_Msk) >> SCB_CPUID_REVISION_Pos) >= CORE_REVISTION_MIN))

#endif/*__HARDWARE_SUPPORT_H__*/
