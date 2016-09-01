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
#ifndef __CORE_GENERIC_H__
#define __CORE_GENERIC_H__

#include <stdint.h>

/* CMSIS core configuration
 * Note: since uVisor is hw-agnostic, we must be conservative. */

/* MPU configuration
 * Always 1, since we require an MPU, unless it's a Kinetis MPU, in which case
 * we use theirs. */
#if defined(ARCH_MPU_ARMv7M)
#define __MPU_PRESENT 1
#elif defined(ARCH_MPU_ARMv8M)
#define __MPU_PRESENT 1
#define __SAUREGION_PRESENT 1
#elif defined(ARCH_MPU_KINETIS)
#define __MPU_PRESENT 0
#else /* MPU configuration */
#error "Unknown MPU architecture. Check your Makefile."
#endif /* MPU configuration */

/* We don't need the SysTick configurtion. */
#define __Vendor_SysTickConfig 0

/* At compile time uVisor does not need to know if the device has an FPU. It
 * will be detected at runtime. */
#define __FPU_PRESENT 0

/* Redefine the macro to a custom variable that is written at startup time by
 * uVisor.
 * This allows us to remove the __NVIC_PRIO_BITS axis from the release binaries
 * matrix */
extern uint8_t g_nvic_prio_bits;
#define __NVIC_PRIO_BITS g_nvic_prio_bits

/* This ensures that the CMSIS header files actively check for our
 * configurations. If a symbol is not configured, a warning is issued. */
#define __CHECK_DEVICE_DEFINES

/* This header file includes the core-specific support for some hardware
 * features. It might override some of the definitions above. */
#include "hardware_support.h"

/* Core interrupts */
typedef enum IRQn {
  NonMaskableInt_IRQn   = -14,
  HardFault_IRQn        = -13,
  MemoryManagement_IRQn = -12,
  BusFault_IRQn         = -11,
  UsageFault_IRQn       = -10,
  SVCall_IRQn           = -5,
  DebugMonitor_IRQn     = -4,
  PendSV_IRQn           = -2,
  SysTick_IRQn          = -1,
} IRQn_Type;

/* Include the CMSIS core header files.
 * The Cortex-M specification must be defined by the release configuration. */
#if defined(CORE_CORTEX_M3)
#include "core_cm3.h"
#elif defined(CORE_CORTEX_M4)
#include "core_cm4.h"
#elif defined(CORE_CORTEX_M81)
#include "core_armv8mml.h"
#else /* Core selection */
#error "Unsupported ARM core. Make sure CORE_* is defined in your workspace."
#endif /* Core selection */

/* Optional MPU headers for the NXP Kinetis MPU */
#ifdef ARCH_MPU_KINETIS
#include "mpu_kinetis.h"
#endif /* ARCH_MPU_KINETIS */

#endif /* __CORE_GENERIC_H__ */
